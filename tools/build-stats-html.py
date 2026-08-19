#!/usr/bin/env python3
"""Creates interactive graph of pybricks-micropython firmware size changes.

Reads recorded sizes from the size-data worktree; setup and refresh
commands are printed on start.
"""

import csv
import os
import re
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


def select(sizes, commits, hub):
    """Selects the useful fields from sorted items. Skips the first diff as well
    as commits that did not change the firmware size.

    Args:
        sizes (dict): firmware size keyed by commit hash, None for failures
        commits (list of git.Commit): mainline commits, oldest first
        hub (str): The hub type.

    Yields:
        (tuple of int, string, string, int, int, bool) The index, commit hash,
            commit message, firmware size, change in size from previous commit
            and whether the size data is missing for this commit
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

        yield i, sha, message, size, diff, missing
        i += 1


def create_plot(size_map, commits, hub):
    print("creating plot for", hub, "at", Path(BUILD_DIR, f"{hub}.html"))

    indexes, shas, messages, sizes, diffs, missing = zip(
        *select(size_map, commits, hub)
    )
    marker_colors = ["red" if m else "#636efa" for m in missing]

    # Find sensible ranges to display by default
    x_end = len(indexes)
    x_start = x_end - 100
    y_end = max(s + 64 for s in sizes[x_start - 1 : x_end])
    y_start = min(s - 64 for s in sizes[x_start - 1 : x_end])
    diff_peak = max([abs(d) + 64 for d in diffs[x_start - 1 : x_end]])

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
        tickvals=[i * 1024 for i in range(256)],
        ticktext=[f"{i}KB" for i in range(256)],
        range=[y_start, y_end],
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
        if (point) {{
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
        create_plot(load_sizes(h), commits, h)


if __name__ == "__main__":
    main()
