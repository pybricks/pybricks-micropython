#!/usr/bin/env python3

import sys

import pygsheets
from pygsheets.exceptions import WorksheetNotFound

SPREADSHEET_ID = "1dQVmmmOafXO-nyWuuZ3Val1cE5dZZkwsVHA9jkU1iT0"


def main():
    if len(sys.argv) != 6:
        raise RuntimeError("Wrong number of command line arguments")
    hub_name, timestamp, build_id, commit_id, size = sys.argv[1:]

    client = pygsheets.authorize(service_account_env_var="GOOGLE_SECRET")
    sheet = client.open_by_key(SPREADSHEET_ID)

    try:
        worksheet = sheet.worksheet_by_title(hub_name)
    except WorksheetNotFound:
        worksheet = sheet.add_worksheet(hub_name)
        worksheet.update_values("A1", [["timestamp", "build", "commit", "size"]])
        worksheet.adjust_column_width(0, pixel_size=150)
        worksheet.adjust_column_width(2, pixel_size=300)

    worksheet.append_table([[timestamp, build_id, commit_id, size]])


if __name__ == "__main__":
    main()
