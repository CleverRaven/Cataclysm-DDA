import functools
import os
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Generator

import click
import orjson


def find_files(paths: list[Path], extensions: tuple[str]) -> Generator[str, None, None]:
    """Recursively search :paths: for files that have :extensions:."""
    for path in paths:
        if os.path.isdir(path):
            for dirpath, _, filenames in os.walk(path):
                for filename in filenames:
                    _, ext = os.path.splitext(filename)
                    if ext in extensions:
                        yield os.path.join(dirpath, filename)
        else:
            if path.suffix in extensions:
                yield path


@functools.total_ordering
@dataclass(init=True, repr=True, slots=True)
class Mutation:
    id_: str
    name: str
    categories: list[str]
    description: str

    def __lt__(self, other) -> bool:
        if not isinstance(other, Mutation):
            return NotImplemented
        return self.name.lower() < other.name.lower()


def write_mutations(mutations: list[Mutation], filename: Path) -> None:
    categories = defaultdict(list)

    for mut in mutations:
        if mut.categories != "NONE":
            for cat in mut.categories:
                categories[cat].append(mut)
        else:
            categories["NONE"].append(mut)

    # Sort categories alphabetically
    categories = dict(sorted(categories.items()))

    # Sort mutations alphabetically in categories
    for mutations in categories.values():
        mutations.sort()

        # Sort each mutation's categories alphabetically
        for mut in mutations:
            mut.categories.sort()

    with open(filename, "w", encoding="utf-8") as fd:
        # Write table of contents
        fd.write("# Table of Contents\n")
        fd.write("- Categories\n")
        for category in categories.keys():
            fd.write(f'  - [{category}](#{category.replace(" ", "-").lower()})\n')

        # Write mutations
        for category, mutations in categories.items():
            fd.write(f"## {category}\n")
            for mut in mutations:
                fd.write(f"### {mut.name}\n")
                fd.write(f"id: `{mut.id_}` \n")
                fd.write(f'categories: `{"`, `".join(mut.categories)}`\n')
                fd.write(f"Description: {mut.description}\n\n")
            fd.write("\n\n")


@click.command()
@click.argument(
    "paths", type=click.Path(exists=True, resolve_path=True, path_type=Path), nargs=-1
)
@click.option(
    "-o",
    "--outfile",
    type=click.Path(resolve_path=True, path_type=Path, dir_okay=False),
    default="mutations.md",
)
def cli(paths, outfile) -> None:
    # The entrypoint to the CLI.
    mutations = []

    for file in find_files(paths, (".json",)):
        with open(file, "rb") as fd:
            jfile = orjson.loads(fd.read())

        if not isinstance(jfile, list):
            continue

        if len(jfile) == 0:
            continue

        if not isinstance(jfile[0], dict):
            continue

        for jo in jfile:
            if jo.get("type") != "mutation":
                continue

            mutations.append(
                Mutation(
                    jo.get("id"),
                    jo.get("name").get("str"),
                    jo.get("category", ["NONE"]),
                    jo.get("description"),
                )
            )

    click.echo(f"Mutations written to {outfile}")
    write_mutations(mutations, outfile)


if __name__ == "__main__":
    cli()
