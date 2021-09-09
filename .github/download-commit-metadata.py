#!/usr/bin/env python3
"""Downloads pybricks-micropython commit data from GitHub and Azure."""

import json
import os
from pathlib import Path

from azure.cosmosdb.table.tableservice import TableService
from gql import gql, Client
from gql.transport.requests import RequestsHTTPTransport

GITHUB_TOKEN = os.environ["GITHUB_TOKEN"]
STORAGE_ACCOUNT = os.environ["STORAGE_ACCOUNT"]
STORAGE_KEY = os.environ["STORAGE_KEY"]
FIRMWARE_SIZE_TABLE = os.environ["FIRMWARE_SIZE_TABLE"]

BUILD_DIR = os.environ.get("BUILD_DIR", "build")

# all past and present hub names
HUBS = ["cityhub", "cplushub", "technichub", "movehub", "primehub", "essentialhub,", "nxt"]

QUERY = """query {
    repository(owner:"pybricks", name:"pybricks-micropython") {
        ref(qualifiedName:"refs/heads/master") {
            target {
                ... on Commit {
                    history(%s) {
                        totalCount
                        pageInfo {
                            hasNextPage
                            endCursor
                        }
                        edges {
                            node {
                                committedDate
                                messageHeadline
                                oid
                            }
                        }
                    }
                }
            }
        }
    }
}"""


def main():
    # grab size data from Azure
    service = TableService(STORAGE_ACCOUNT, STORAGE_KEY)
    sizes = {
        item["RowKey"]: {k: v for k, v in item.items() if k in HUBS}
        for item in service.query_entities(
            FIRMWARE_SIZE_TABLE,
            filter="PartitionKey eq 'size'",
            select=",".join(["RowKey"] + HUBS),
        )
    }

    # merge cplushub into technichub
    for v in sizes.values():
        if v["technichub"] is None:
            v["technichub"] = v["cplushub"]
        del v["cplushub"]

    # grab commit data from GitHub
    transport = RequestsHTTPTransport(
        url="https://api.github.com/graphql",
        headers={"Authorization": f"bearer {GITHUB_TOKEN}"},
    )
    client = Client(transport=transport, fetch_schema_from_transport=True)

    query = gql(QUERY % "first:100")
    commits = []
    no_sizes = {h: None for h in HUBS if h != "cplushub"}
    while True:
        result = client.execute(query)
        history = result["repository"]["ref"]["target"]["history"]
        for edge in history["edges"]:
            node = edge["node"]
            node["firmwareSize"] = sizes.get(node["oid"], no_sizes)
            commits.append(node)

        if not history["pageInfo"]["hasNextPage"]:
            break

        cursor = history["pageInfo"]["endCursor"]
        query = gql(QUERY % f'first:100, after:"{cursor}"')

    Path(BUILD_DIR).mkdir(parents=True, exist_ok=True)
    with open(Path(BUILD_DIR, "commits.json"), "w") as f:
        json.dump(commits, f, indent=4)


if __name__ == "__main__":
    main()
