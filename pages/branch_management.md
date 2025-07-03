---
layout: page
title: ""
---

## Example Workflow

#### Setup your environment

*(This only needs to be done once.)*

1. Fork this repository here on GitHub.

2. Clone your fork locally.

        $ git clone https://github.com/YOUR_USERNAME/Cataclysm-DDA.git
        # Clones your fork of the repository into the current directory in terminal

3. Add this repository as a remote.

        $ cd Cataclysm-DDA
        # Changes the active directory in the prompt to the newly cloned "Cataclysm-DDA" directory
        $ git remote add -f upstream https://github.com/CleverRaven/Cataclysm-DDA.git
        # Assigns the original repository to a remote called "upstream"

#### Update your `master` branch

1. Make sure you have your `master` branch checked out.

        $ git checkout master

2. Pull the changes from the `upstream/master` branch.

        $ git pull --ff-only upstream master
        # gets changes from "master" branch on the "upstream" remote

 * Note: If this gives you an error, it means you have committed directly to your local `master` branch. [Click here for instructions on how to fix this issue](#why-does-git-pull---ff-only-result-in-an-error).

#### Make your changes

0. Update your `master` branch, if you haven't already.

1. For each new feature or bug fix, create a new branch.

        $ git branch new_feature
        # Creates a new branch called "new_feature"
        $ git checkout new_feature
        # Makes "new_feature" the active branch

2. Once you've committed some changes locally, you need to push them to your fork here on GitHub.

        $ git push origin new_feature
        # origin was automatically set to point to your fork when you cloned it


3. Once you're finished working on your branch and have committed and pushed all your changes, submit a pull request from your `new_feature` branch to this repository's `master` branch.

 * Note: any new commits to the `new_feature` branch on GitHub will automatically be included in the pull request, so make sure to only commit related changes to the same branch.

## Pull Request Notes
* Mark pull requests that are still being worked on by adding [WIP] before the title
    * When a pull request is ready to be reviewed remove the [WIP]
* Mark pull requests that need commenting/testing by others with [CR] before the title
    * [WIP] has higher precedence than [CR]. Feel free to remove [CR] when you feel you got enough information
* If the pull request fixes a issue listed on github, include "fixes #???" into the text, where ??? is the number of the issue. This automatically closes the issue when the PR is pulled in, and allows mergers to work slightly faster. For further details see issue #2419.

#### PR Tags
If you file a PR but you're still working on it, please add a [WIP] before the title text. This will tell the reviewers that you still intend to add more to the PR and we don't need to review it yet. When it's ready to be reviewed by a merger just edit the title text to remove the [WIP].

If you are also looking for suggestions then mark it with [CR] - "comments requested". You can use both [WIP] and [CR] to indicated that you need opinion/code review/suggestions to continue working (e.g. "[WIP] [CR] Super awesome big feature"). Feel free to remove [CR] when you feel you got enough information to proceed.

This can help speed up our review process by allowing us to only review the things that are ready for it and will prevent anything that isn't completely ready from being merged in.

One more thing: when marking your PR as closing, fixing, or resolving issues, please include "XXXX #???" somewhere in the description, where XXX is on this list:
close
closes
closed
fix
fixes
fixed
resolve
resolves
resolved
The "???" is the issue number. This automatically closes the issue when the PR is pulled in, and allows merges to work slightly faster. To close multiple issues format it as "XXXX #???, XXXX#???".
