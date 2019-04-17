---
layout: page
title: "Contribution Guidelines"
permalink: /contributions/
---

# Contribute

**Opening new issue?** Please read [issues guidelines]({{ site.baseurl }}{% link pages/issues.md %}) first.

**Contributing code?** Check [code contribution guidelines]({{ site.baseurl }}{% link pages/code_contributions.md %}).

**Investigating bugs?** [debugging commands]({{ site.baseurl }}{% link pages/debugging.md %}) can help.

**Need help with git?** [git howto]({{ site.baseurl }}{% link pages/branch_management.md %}) might be what you're looking for.

Contributing code or data to Cataclysm: Dark Days Ahead is easy - simply fork the repository here on GitHub, make your changes, and then send us a pull request.

Cataclysm:Dark Days Ahead is released under the Creative Commons Attribution ShareAlike 3.0 license.  The code and content of the game is free to use, modify, and redistribute for any purpose whatsoever.  See https://creativecommons.org/licenses/by-sa/3.0/ for details.
This means any contribution you make to the project will also be covered by the same license, and this license is irrevocable.

## Guidelines

There are a couple of guidelines we suggest sticking to:

* Add the repository as an `upstream` remote.
* Keep your `master` branch clean. This means you can easily pull changes made to this repository into yours.
* Create a new branch for each new feature or set of related bug fixes.
* Never merge from your local branches into your `master` branch. Only update that by pulling from `upstream/master`.

## Frequently Asked Questions

#### Why does `git pull --ff-only` result in an error?

If `git pull --ff-only` shows an error, it means that you've committed directly to your local `master` branch. To fix this, we create a new branch with these commits, find the point at which we diverged from `upstream/master`, and then reset `master` to that point.

    $ git pull --ff-only upstream master
    From https://github.com/CleverRaven/Cataclysm-DDA
     * branch            master     -> FETCH_HEAD
    fatal: Not possible to fast-forward, aborting.
    $ git branch new_branch master          # mark the current commit with a tmp branch
    $ git merge-base master upstream/master
    cc31d0... # the last commit before we committed directly to master
    $ git reset --hard cc31d0....
    HEAD is now at cc31d0... ...

Now that `master` has been cleaned up, we can easily pull from `upstream/master`, and then continue working on `new_branch`.

    $ git pull --ff-only upstream master
    # gets changes from the "upstream" remote for the matching branch, in this case "master"
    $ git checkout new_branch
