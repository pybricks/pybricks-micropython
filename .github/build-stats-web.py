#!/usr/bin/env python3
"""Creates interactive graph of pybricks-micropython firmware size changes."""

import json
import os
import re
from pathlib import Path

from plotly import graph_objects as go
from plotly.offline import plot
from plotly.subplots import make_subplots

BUILD_DIR = os.environ.get("BUILD_DIR", "build")

# Number of digits of hash to display
HASH_SIZE = 7

HUBS = ["cityhub", "technichub", "movehub", "primehub", "essentialhub", "nxt"]

GITHUB_REPO_URL = "https://github.com/pybricks/pybricks-micropython"


def select(commits, hub):
    """Selects the useful fields from sorted items. Skips the first diff as well
    as commits that did not change the firmware size.

    Args:
        commits (list of dict): contents commits.json from download.py
        hub (str): The hub type.

    Yields:
        (tuple of int, string, string, int, int) The index, commit hash, commit
            message, firmware size and change in size from previous commit
    """
    prev_size = 0
    for i, commit in enumerate(reversed(commits)):
        sha = commit["oid"][:HASH_SIZE]
        message = commit["messageHeadline"]
        date = commit["committedDate"]
        size = commit["firmwareSize"][hub]
        diff = 0
        if size is None:
            size = 0
        else:
            if prev_size != 0:
                diff = size - prev_size
                message = f"{diff:+}<br />{message}<br />{date}"
            prev_size = size

        yield i, sha, message, size, diff


def create_plot(commits, hub):
    indexes, shas, messages, sizes, diffs = zip(*select(commits, hub))

    # Find sensible ranges to display by default
    x_end = len(indexes)
    x_start = x_end - 100
    y_end = max([s for s in sizes[x_start - 1 : x_end]])
    y_start = min([s for s in sizes[x_start - 1 : x_end]])
    diff_peak = max([abs(d) for d in diffs[x_start - 1 : x_end]])

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
            hoverinfo="text+y",
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


def main():
    with open(Path(BUILD_DIR, "commits.json"), "r") as f:
        commits = json.load(f)

    for h in HUBS:
        create_plot(commits, h)


if __name__ == "__main__":
    main()
