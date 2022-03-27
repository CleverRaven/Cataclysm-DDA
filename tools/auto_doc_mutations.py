import functools
import os
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Generator

import click
import orjson


def find_files(
    paths: list[Path], extensions: tuple[str], include_mods=False
) -> Generator[str, None, None]:
    """Recursively search :paths: for files that have :extensions:.

    If :include_mods: is False (the default), ignore all files that are in a
    subdirectory of a directory containing a modinfo.json file. If
    :include_mods: is True, yield modinfo.json before all other files,
    if it exists.
    """
    for path in paths:
        if os.path.isdir(path):
            for dirpath, dirnames, filenames in os.walk(path):
                # Ignore all subdirectories and files inside a mod.
                if not include_mods and "modinfo.json" in filenames:
                    dirnames.clear()
                    filenames.clear()

                # Yield modinfo.json before all other files, if it exists.
                if include_mods and "modinfo.json" in filenames:
                    # More than one modinfo.json is not allowed
                    if filenames.count("modinfo.json") > 1:
                        dirnames.clear()
                        filenames.clear()
                        click.echo(
                            f"More than one modinfo.json found in {dirpath}.",
                            err=True,
                        )
                    else:
                        filenames.remove("modinfo.json")
                        yield os.path.join(dirpath, "modinfo.json")

                for filename in filenames:
                    _, ext = os.path.splitext(filename)
                    if ext in extensions:
                        yield os.path.join(dirpath, filename)
        else:
            if path.suffix in extensions:
                yield path


@functools.total_ordering
@dataclass(init=True, repr=True)
class Mutation:
    id_: str
    name: str
    categories: list[str]
    description: str
    mod: str
    copied: bool = False

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
            fd.write(
                f'  - [{category}](#{category.replace(" ", "-").lower()})\n'
            )
        fd.write("\n\n")

        # Write mutations
        for category, mutations in categories.items():
            fd.write(f"## {category}\n")
            for mut in mutations:
                fd.write(f"### {mut.name}\n")
                fd.write(f"src: `{mut.mod}`\n")
                fd.write(f"id: `{mut.id_}` \n")
                fd.write(f'categories: `{"`, `".join(mut.categories)}`\n')
                fd.write(f"Description: {mut.description}\n")
            fd.write("\n\n")


def extract_mutations(file: Path, mod: str = "dda") -> list[Mutation]:
    """Extract mutations from :file:."""

    with open(file, "rb") as fd:
        jfile = orjson.loads(fd.read())

    if not isinstance(jfile, list):
        return []

    if len(jfile) == 0:
        return []

    mutations = []

    for jo in jfile:
        if not isinstance(jo, dict):
            continue

        if jo.get("type") != "mutation":
            continue

        mutation = Mutation(
            id_=jo.get("id"),
            name="",
            categories=jo.get(
                "category", jo.get("extend", {}).get("category", ["NONE"])
            ),
            description=jo.get("description"),
            mod=mod,
        )

        name = jo.get("name")
        if isinstance(name, dict):
            name = name.get("str")
        elif name is None and jo.get("copy-from"):
            name = "SHOULD_BE_COPIED_FROM"
            mutation.copied = True
        elif not isinstance(name, str):
            click.echo(
                f"Invalid mutation name: {name} in file: {file}", err=True
            )
            sys.exit()

        mutation.name = name
        mutations.append(mutation)

    return mutations


@dataclass(init=True, repr=True)
class Mod:
    id_: str
    mutations: list[Mutation]
    dependencies: list[str]


@click.command()
@click.argument(
    "paths",
    type=click.Path(exists=True, resolve_path=True, path_type=Path),
    nargs=-1,
)
@click.option(
    "-o",
    "--outfile",
    type=click.Path(resolve_path=True, path_type=Path, dir_okay=False),
    default="mutations.md",
    help="""Specify the filepath to write the documentation for DDA to.
    If mods are included, their documentation will be written to the same
    directory.""",
    show_default=True,
)
@click.option(
    "--include-mods / --exclude-mods",
    default=False,
    help="""Include (or exclude) mutations defined in mods.
    A mod is a file which is contained in a subdirectory of a
    directory including a modinfo.json file.""",
    show_default=True,
)
def cli(paths, outfile, include_mods) -> None:
    """Search PATHS for JSON files and generate documentation for mutations."""
    # The entrypoint to the CLI.

    if not include_mods:
        mutations = []

        for file in find_files(paths, (".json",), include_mods=False):
            mutations.extend(extract_mutations(file))

        write_mutations(mutations, outfile)
        click.echo(f"Mutations written to {outfile}")
    else:
        dda = Mod("dda", [], [])
        mod_stack: list[tuple[Mod, str]] = [(dda, "PLACEHOLDER")]
        mods: dict[str, Mod] = {"dda": dda}

        def move_stack(
            file, mod_stack: list[tuple[Mod, str]]
        ) -> list[tuple[Mod, str]]:
            if len(mod_stack) == 1:
                return mod_stack
            if mod_stack[-1][1] in Path(file).parents:
                return mod_stack
            else:
                return move_stack(file, mod_stack[:-1])

        for file in find_files(paths, (".json",), include_mods=True):
            if os.path.basename(file) != "modinfo.json":
                # Move mods who we have recursed out of off the stack.
                mod_stack = move_stack(file, mod_stack)
                mod_stack[-1][0].mutations.extend(
                    extract_mutations(file, mod_stack[-1][0].id_)
                )

            elif os.path.split(os.path.split(file)[0])[1] == "dda":
                # Skip DDA
                continue
            else:
                # Add a new mod to the stack.
                with open(file, "rb") as fd:
                    jfile = orjson.loads(fd.read())

                # modinfo.json supports two formats:
                # - An array containing at least one object, the first of which
                # contains info about the mod.
                # - A single top-level object.
                if isinstance(jfile, list) and len(jfile) > 0:
                    mod_info = jfile[0]
                else:
                    mod_info = jfile

                if isinstance(mod_info, dict) and all(
                    (mod_info.get("id"), mod_info.get("dependencies"))
                ):
                    new_mod = Mod(mod_info["id"], [], mod_info["dependencies"])
                    mod_stack.append((new_mod, Path(os.path.dirname(file))))
                    mods[new_mod.id_] = new_mod
                else:
                    click.echo(f"{file} is an invalid modinfo.json file.")
                    sys.exit()

        # Resolve mutations which use copy-from.
        for id_, mod in mods.items():
            if not mod.dependencies:
                continue

            for mut in (mut for mut in mod.mutations if mut.copied is True):
                for dep in mod.dependencies:
                    if not mods.get(dep):
                        click.echo(
                            f"Mod {mod.id_} is missing dependency {dep}"
                        )
                        continue

                    for m in mods[dep].mutations:
                        if m.id_ == mut.id_:
                            mut.description = m.description
                            mut.name = m.name
                            mut.mod = m.mod

                            if mut.categories != ["NONE"] and m.categories != [
                                "NONE"
                            ]:
                                mut.categories += m.categories
                            elif m.categories != ["NONE"]:
                                mut.categories = m.categories

        for _, mod in mods.items():
            try:
                if not mod.mutations:
                    continue
            except AttributeError:
                breakpoint()

            if mod.id_ == "dda":
                writepath = outfile
            else:
                writepath = outfile.parent.joinpath(mod.id_ + ".md")

            write_mutations(mod.mutations, writepath)
            click.echo(
                f"Mutations for mod [ {mod.id_} ] written to {writepath}"
            )


if __name__ == "__main__":
    cli()
