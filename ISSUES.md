# Issues

## How to create new issues properly

GitHub issues are used for everything from bug reporting to suggesting long-term ideas. You can make everything run much smoother by following some simple rules.

### Rule zero

Always give your issue a meaningful title as this is the first thing anyone will see.

Note: `[CR]` and `[WIP]` "tags" are meaningful only for PRs. All open issues by definition are request for comments and work in progress.

### Bug reports

Before you submit a bug always search the current list of issues to see if it has been reported already.

Your bug report has to include:

- On what OS did you experience the problem (Windows, Linux, OS X etc.)
- What version were you playing:
 - Tiles or Curses (text-based)
 - Version string (preferably full version e.g. "0.C-4547-g3f1c109", or Jenkins build number e.g. 3245)
- Description of the problem written in a way that enables anyone to try and recreate it

Your bug report may include:

- Screenshot(s) as some things are best explained visually
- Save file (e.g. link to a dropbox upload)

Bonus points for:

- Checking if the bug exists under latest experimental build
- Checking if it is OS specific

The OS and CDDA version are very important - with the pace of changes here it is possible the bug you have encountered has already been fixed. After that reproducibility is the key, so write your report with all the necessary details.

### Enhancements and addition ideas

We have hundreds of issues open - most of them are ideas and suggestions. If you have a general idea or anything that can't be easily described in terms of *current* code changes you'd be better off suggesting it in the appropriate section of [the forum](https://discourse.cataclysmdda.org/). You'll also get much broader exposure for your idea there. After developing a polished idea on the forum, it should be easy to make a GitHub issue for it.

Please first search if something like what you have on mind has been already proposed. If so, feel free to join the discussion! If your idea is related but sufficiently different, open a new issue and refer to the older discussion (use GitHub's `#issue_number` reference system).

Remember to take part in the discussion of your suggestions.

### Labels

You can update issues with labels using comment-commands. First, open a PR adding your username to a list of allowed users [here](.github/comment-commands.yml), under the `active` section. Then you can add labels to issues, such as marking them confirmed with `/confirmed`, or requesting help with `/help-wanted`. A list of comments you can use is under the `keywords` section [here](.github/comment-commands.yml), and what each of these comment-commands do is described [here](.github/workflows/comment-commands.yml).

### Questions

You should direct your questions to the forum or ask on IRC. You should also read the included documentation and additional text files, e.g. [COMPILING.md](doc/COMPILING/COMPILING.md) if you have problems building.

## Bounties

Placing a bounty *does not* necessarily mean that change will be incorporated into the main game. Please coordinate in the issue you intend to place a bounty on to determine if it is a change the project will accept, and keep in mind that placing a bounty will not confer special status on the issue. A good way of thinking about bounties is as encouragement for contributors to work on a particular issue, and certainly not as "paying for features".

## Issue resolution

We do not assign people to issues. If you plan to work on a bug fix or a validated idea feel free to just comment about that. Actual PRs are of much greater value than any assignments. In general the first correct PR about something will be the PR that will get merged, but remember: we are using Git - you can collaborate with someone else easily by sending them patches or PRs against their PR branch.
