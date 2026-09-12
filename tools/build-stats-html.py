#!/usr/bin/env python3
"""Creates interactive graph of pybricks-micropython firmware size changes.

Reads recorded sizes from the size-data worktree; setup and refresh
commands are printed on start.
"""

import csv
import os
import re
import subprocess
import sys
from pathlib import Path

import git

from plotly import graph_objects as go
from plotly.offline import plot
from plotly.subplots import make_subplots

# size-data worktree checked out inside this repo (gitignored), same as CI
SIZE_DATA_DIR = "size-data"
BUILD_DIR = os.environ.get("BUILD_DIR", os.path.join(SIZE_DATA_DIR, "build"))

# branch holding the size data
SIZE_BRANCH = os.environ.get("SIZE_BRANCH", "size-data")

# Number of digits of hash to display
HASH_SIZE = 8

HUBS = [
    "movehub",
    "cityhub",
    "technichub",
    "essentialhub",
    "primehub",
    "nxt",
    "ev3",
    "buildhat",
]

# Linker script holding the space available to the firmware image, and the
# MEMORY region within it that the image is linked into. The scripts have moved
# and the regions have been renamed over the years, so both are listed newest
# first and the first one found at a given commit wins. EV3 is absent because it
# loads the image from an SD card into 64 MiB of DDR, so there is no limit worth
# plotting.
PLATFORM_LD = {
    "movehub": (
        [
            "lib/pbio/platform/move_hub/platform.ld",
            "bricks/movehub/move_hub.ld",
            "bricks/MOVEHUB/move_hub.ld",
            "bricks/MOVEHUB/move-hub.ld",
            "bricks/MOVEHUB/stm32f070.ld",
        ],
        ["FLASH_FIRMWARE", "FLASH"],
    ),
    "cityhub": (
        [
            "lib/pbio/platform/city_hub/platform.ld",
            "bricks/cityhub/city_hub.ld",
            "bricks/HUB4/hub4.ld",
        ],
        ["FLASH_FIRMWARE", "FLASH"],
    ),
    "technichub": (
        [
            "lib/pbio/platform/technic_hub/platform.ld",
            "bricks/technichub/technic_hub.ld",
            "bricks/cplushub/cplus_hub.ld",
        ],
        ["FLASH_FIRMWARE", "FLASH"],
    ),
    "essentialhub": (
        [
            "lib/pbio/platform/essential_hub/platform.ld",
            "bricks/essentialhub/essential_hub.ld",
        ],
        ["FLASH_FIRMWARE", "FLASH"],
    ),
    # the recorded size is that of the prime_hub_f4 variant
    "primehub": (
        [
            "lib/pbio/platform/prime_hub_f4/platform.ld",
            "lib/pbio/platform/prime_hub/platform.ld",
            "bricks/primehub/prime_hub.ld",
        ],
        ["FLASH_FIRMWARE", "FLASH"],
    ),
    # the rest of the ROM is the user file system, which starts where the
    # firmware ends
    "nxt": (
        [
            "lib/pbio/platform/nxt/platform.ld",
            "bricks/nxt/nxt.ld",
        ],
        ["rom", "ROM"],
    ),
    # the image is loaded into RAM, which used to be shortened to keep it clear
    # of the memory that the Build HAT bootloader uses while loading it. That is
    # now spelled out as __bootloader_ram_start instead, so that the RAM the
    # bootloader leaves behind can be reclaimed for the GC heap.
    "buildhat": (
        ["lib/pbio/platform/build_hat/rpi_build_hat.ld"],
        ["RAM"],
    ),
}

# Color of the line showing the space available to the firmware
LIMIT_COLOR = "#d62728"

# How full the firmware has to be before the limit is worth keeping in view by
# default. The hubs with room to spare would otherwise have the recent history
# that the graph is for squashed into a thin band at the bottom, so their limit
# is only seen by zooming out.
LIMIT_IN_VIEW = 0.9

GITHUB_REPO_URL = "https://github.com/pybricks/pybricks-micropython"

INITIAL_COMMIT = "281d6ffa6a182f502e81ae0c4ff9b71f6e674f71"
PYBRICKS_BRANCH = "origin/master"
PYBRICKS_PATH = os.environ.get("PYBRICKS_PATH", ".")

try:
    pybricks = git.Repo(PYBRICKS_PATH)
except Exception as e:
    print(f"Repository not found at '{PYBRICKS_PATH}':", e)
    print("try setting the PYBRICKS_PATH environment variable")
    sys.exit(1)

assert not pybricks.bare, "Repository not found"


def evaluate(expression):
    """Evaluates a linker script expression such as "1M - 64K" or "0x2003C000".

    Args:
        expression (str): the expression

    Returns:
        (int) The value in bytes, or None if it is not just arithmetic.
    """
    # "DEFINED(SYMBOL) ? a : b" picks the layout of a build that defines the
    # symbol on the command line, such as the SPIKE Essential dual boot layout
    # that was never released, so take the plain branch
    if "?" in expression:
        expression = expression.split("?", 1)[1].split(":", 1)[1]

    # the K and M suffixes of the GNU linker are binary multiples
    expression = re.sub(r"(\d)\s*[Kk]\b", r"\1*1024", expression)
    expression = re.sub(r"(\d)\s*[Mm]\b", r"\1*1024*1024", expression)

    if not re.fullmatch(r"[0-9a-fA-FxX+\-*/() ]+", expression):
        return None

    return eval(expression, {"__builtins__": {}})


def parse_memory(script):
    """Parses the MEMORY block of a linker script.

    Args:
        script (str): contents of the linker script

    Returns:
        (dict) origin and length in bytes, keyed by region name
    """
    script = re.sub(r"/\*.*?\*/", "", script, flags=re.DOTALL)

    block = re.search(r"\bMEMORY\s*\{(.*?)\}", script, flags=re.DOTALL)
    if not block:
        return {}

    regions = {}

    for name, origin, length in re.findall(
        r"(\w+)\s*(?:\([^)]*\))?\s*:\s*ORIGIN\s*=\s*(.+?),\s*LENGTH\s*=\s*(.+)",
        block.group(1),
    ):
        regions[name] = (evaluate(origin), evaluate(length))

    return regions


def parse_limit(script, hub):
    """Finds the space available to the firmware image in a linker script.

    Args:
        script (str): contents of the linker script
        hub (str): the hub type

    Returns:
        (int) The size in bytes, or None if the script could not be read.
    """
    regions = parse_memory(script)

    if hub == "buildhat":
        # where the image has to stop is a symbol rather than the end of the
        # region, see the comment on PLATFORM_LD
        stop = re.search(
            r"__bootloader_ram_start\s*=\s*(\S+?)\s*;?\s*$",
            script,
            flags=re.MULTILINE,
        )
        if stop and "RAM" in regions:
            return evaluate(stop.group(1)) - regions["RAM"][0]

    for name in PLATFORM_LD[hub][1]:
        if name in regions:
            return regions[name][1]

    return None


def load_limits(commits, hub):
    """Reads the space available to the firmware at each commit.

    Args:
        commits (list of git.Commit): mainline commits, oldest first
        hub (str): the hub type

    Returns:
        (dict) available space in bytes, keyed by commit hash, missing for
            commits where no linker script was found
    """
    if hub not in PLATFORM_LD:
        return {}

    paths = PLATFORM_LD[hub][0]

    # ask for every candidate path at every commit in one go, since a lookup
    # per commit would take minutes over the whole history
    query = "".join(f"{c.hexsha}:{p}\n" for c in commits for p in paths)
    checked = subprocess.run(
        ["git", "cat-file", "--batch-check"],
        cwd=PYBRICKS_PATH,
        input=query,
        capture_output=True,
        text=True,
        check=True,
    ).stdout.splitlines()

    # the answers come back in the order asked, so the first path that exists
    # in a commit's group is the one in use at that commit
    blobs = {}

    for i, commit in enumerate(commits):
        for line in checked[i * len(paths) : (i + 1) * len(paths)]:
            if not line.endswith(" missing"):
                blobs[commit.hexsha] = line.split()[0]
                break

    # each script is parsed once, however many commits share it
    limits = {}

    for blob in set(blobs.values()):
        limits[blob] = parse_limit(pybricks.git.cat_file("blob", blob), hub)

    return {sha: limits[blob] for sha, blob in blobs.items() if limits[blob]}


def select(sizes, limits, commits, hub):
    """Selects the useful fields from sorted items. Skips the first diff as well
    as commits that did not change the firmware size.

    Args:
        sizes (dict): firmware size keyed by commit hash, None for failures
        limits (dict): available space keyed by commit hash, missing if unknown
        commits (list of git.Commit): mainline commits, oldest first
        hub (str): The hub type.

    Yields:
        (tuple of int, string, string, int, int, bool, int) The index, commit
            hash, commit message, firmware size, change in size from previous
            commit, whether the size data is missing for this commit and the
            space available to the firmware
    """
    prev_size = 0
    i = 0

    for commit in commits:
        size = sizes.get(commit.hexsha)

        # skip leading commits before the first recorded size
        if size is None and prev_size == 0:
            continue

        sha = commit.hexsha[:HASH_SIZE]
        message = commit.summary
        date = commit.committed_datetime.strftime("%Y-%m-%d")
        diff = 0
        missing = size is None

        if missing:
            size = prev_size
            message = f"no data<br />{message}<br />{date}"
        else:
            if prev_size != 0:
                diff = size - prev_size
                message = f"{diff:+}<br />{message}<br />{date}"
            prev_size = size

        yield i, sha, message, size, diff, missing, limits.get(commit.hexsha)
        i += 1


def label_limit(value):
    """Describes an amount of available space, in both units the graph uses.

    Args:
        value (int): the size in bytes, or None if it is not known

    Returns:
        (str) The description, empty if the size is not known.
    """
    if not value:
        return ""

    return f"{value // 1024}KiB ({value} bytes) available"


def segments(limits):
    """Splits a series of limits into runs of the same value.

    Args:
        limits (list of int): available space at each commit, None if unknown

    Yields:
        (tuple of int, int, int) The first and last index of the run and the
            value it holds. Runs of unknown values are skipped.
    """
    start = 0

    for i, value in enumerate(limits):
        if value == limits[start]:
            continue

        if limits[start]:
            yield start, i - 1, limits[start]

        start = i

    if limits[start]:
        yield start, len(limits) - 1, limits[start]


def y_ticks(y_start, y_end, y_max):
    """Picks y axis ticks at a whole number of kibibytes.

    The step is chosen so that the range visible by default gets about a dozen
    labels, but ticks are generated for the whole plot so that zooming out
    still shows them.

    Args:
        y_start (int): bottom of the range visible by default
        y_end (int): top of the range visible by default
        y_max (int): largest value that can be shown by zooming out

    Returns:
        (tuple of list) The tick values and their labels.
    """
    step = 1024
    while (y_end - y_start) / step > 16:
        step *= 2

    # the whole plot shares one set of ticks, so coarsen them again rather than
    # let a hub with a lot of history draw hundreds of labels when zoomed out
    while y_max / step > 256:
        step *= 2

    values = list(range(0, y_max + step, step))
    return values, [f"{v // 1024}KiB" for v in values]


def create_plot(size_map, limit_map, commits, hub):
    print("creating plot for", hub, "at", Path(BUILD_DIR, f"{hub}.html"))

    indexes, shas, messages, sizes, diffs, missing, limits = zip(
        *select(size_map, limit_map, commits, hub)
    )
    marker_colors = ["red" if m else "#636efa" for m in missing]

    # Find sensible ranges to display by default
    x_end = len(indexes)
    x_start = x_end - 100
    y_end = max(s + 64 for s in sizes[x_start - 1 : x_end])
    y_start = min(s - 64 for s in sizes[x_start - 1 : x_end])
    diff_peak = max([abs(d) + 64 for d in diffs[x_start - 1 : x_end]])

    # leave the limit in view by default only once the firmware is getting
    # close to filling the space available
    visible_limits = [x for x in limits[x_start - 1 : x_end] if x]
    if visible_limits and y_end > min(visible_limits) * LIMIT_IN_VIEW:
        y_end = max(visible_limits) + 1024

    known_limits = [x for x in limits if x]
    tickvals, ticktext = y_ticks(y_start, y_end, max([max(sizes)] + known_limits))

    # Create the figure with two subplots
    fig = make_subplots(rows=2, cols=1)
    fig.update_layout(
        showlegend=False,
        title_text=f"Pybricks {hub} firmware size",
        titlefont=dict(size=36),
        dragmode="zoom",
    )
    fig.update_xaxes(showticklabels=False, range=[x_start, x_end])

    # Add and configure size plot
    fig.append_trace(
        go.Scatter(
            x=indexes,
            y=sizes,
            name="Size",
            line={"shape": "hv"},
            mode="lines+markers",
            marker={"color": marker_colors},
            hovertext=messages,
            hoverinfo="y+text",
            customdata=shas,
        ),
        row=1,
        col=1,
    )
    fig.update_yaxes(
        row=1,
        exponentformat="none",
        tickmode="array",
        tickvals=tickvals,
        ticktext=ticktext,
        range=[y_start, y_end],
    )

    # Add the space available to the firmware, which steps whenever the
    # linker script changed
    if known_limits:
        fig.append_trace(
            go.Scatter(
                x=indexes,
                y=limits,
                name="Available",
                line={"shape": "hv", "color": LIMIT_COLOR, "dash": "dash"},
                mode="lines",
                hovertext=[label_limit(v) for v in limits],
                hoverinfo="text",
            ),
            row=1,
            col=1,
        )

        # Label each step, keeping the label of the step that is showing when
        # the page opens inside the default view rather than off to the left
        for begin, end, value in segments(limits):
            fig.add_annotation(
                row=1,
                col=1,
                x=max(begin, x_start) if end > x_start else begin,
                y=value,
                text=label_limit(value),
                showarrow=False,
                xanchor="left",
                yanchor="bottom",
                font={"color": LIMIT_COLOR},
            )

    # Add and configure diff plot
    fig.append_trace(
        go.Bar(
            x=indexes,
            y=diffs,
            hovertext=messages,
            hoverinfo="text",
            name="Delta",
            customdata=shas,
        ),
        row=2,
        col=1,
    )
    fig.update_yaxes(row=2, range=[-diff_peak, diff_peak])

    # Export plot
    # https://community.plot.ly/t/hyperlink-to-markers-on-map/17858/6

    # Get HTML representation of plotly.js and this figure
    plot_div = plot(fig, output_type="div", include_plotlyjs="cdn")

    # Get id of html div element that looks like
    # <div id="301d22ab-bfba-4621-8f5d-dc4fd855bb33" ... >
    res = re.search('<div id="([^"]*)"', plot_div)
    div_id = res.groups()[0]

    # Build JavaScript callback for handling clicks
    # and opening the URL in the trace's customdata
    js_callback = f"""
    <script>
    const base_url = '{GITHUB_REPO_URL}/commit/';
    const plot_element = document.getElementById('{div_id}');
    plot_element.on('plotly_click', function(data) {{
        console.debug(data);
        const point = data.points[0];
        // the line showing the available space has no commit behind it
        if (point && point.customdata) {{
            console.debug(point.customdata);
            window.open(base_url + point.customdata);
        }}
    }})
    </script>
    """

    # Build HTML string
    html_str = f"""
    <html>
    <head>
    </head>
    <body>
    {plot_div}
    {js_callback}
    </body>
    </html>
    """

    # Write out HTML file
    with open(Path(BUILD_DIR, f"{hub}.html"), "w") as f:
        f.write(html_str)


def load_sizes(hub):
    """Loads a hub's sizes from the size-data worktree.

    Returns:
        (dict) firmware size keyed by commit hash, None for recorded failures
    """
    path = os.path.join(PYBRICKS_PATH, SIZE_DATA_DIR, f"{hub}.csv")
    with open(path, newline="") as f:
        return {row[0]: int(row[1]) if row[1] else None for row in csv.reader(f)}


def main():
    if not os.path.isdir(os.path.join(PYBRICKS_PATH, SIZE_DATA_DIR)):
        print("Size data worktree not found. Set it up once with:")
        print(f"    git fetch origin {SIZE_BRANCH}")
        print(
            f"    git worktree add -b {SIZE_BRANCH}"
            f" {SIZE_DATA_DIR} origin/{SIZE_BRANCH}"
        )
        sys.exit(1)

    print(
        f"\nTo refresh size data first: git -C {SIZE_DATA_DIR} pull origin {SIZE_BRANCH}\n"
    )

    # the tree has multiple independent histories that have been merged
    # we only want commits that belong the the mainline
    commits = list(
        pybricks.iter_commits(
            f"{INITIAL_COMMIT}..{PYBRICKS_BRANCH}", ancestry_path=True
        )
    )
    commits.reverse()  # oldest first

    Path(BUILD_DIR).mkdir(parents=True, exist_ok=True)

    for h in HUBS:
        create_plot(load_sizes(h), load_limits(commits, h), commits, h)


if __name__ == "__main__":
    main()
