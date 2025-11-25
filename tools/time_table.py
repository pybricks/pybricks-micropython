"""
Create chronological lists of commits with clickable hash links as HTML table.
"""

from datetime import datetime
from git import Repo

# Configuration constants
START_DATE = datetime(2025, 3, 11, 0, 0)
END_DATE = datetime(2025, 11, 24, 0, 0)
REPO_PATH = "."
FILTER_AUTHOR_NAME = None
USE_AUTHORED_DATE = False
BASE_URL = "https://github.com/pybricks/pybricks-micropython/commit/"


def get_datetime(commit):
    return commit.authored_datetime if USE_AUTHORED_DATE else commit.committed_datetime


# Get commits for the given date ranges.
repo = Repo(REPO_PATH)
commits = list(repo.iter_commits(since=START_DATE, until=END_DATE))
commits.sort(key=get_datetime)

EMPTY_DATE = "          "
last_date = EMPTY_DATE

# Print commits as html table with links.
print("<html><table>")
for c in commits:
    if FILTER_AUTHOR_NAME and c.author.name != FILTER_AUTHOR_NAME:
        continue
    message = c.message.strip().split("\n")[0]
    date = get_datetime(c).date()
    if date == last_date:
        date = EMPTY_DATE
        day = ""
    else:
        last_date = date
        day = ("Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun")[
            get_datetime(c).weekday()
        ]
    sha = c.hexsha[0:7]
    link = f'<a href="{BASE_URL}{sha}">{sha}</a>'
    print(
        f"<tr><td>{date}</td><td>{day}</td><td></td><td></td><td>{link}</td><td>{message}</td></tr>"
    )
print("</table></html>")
