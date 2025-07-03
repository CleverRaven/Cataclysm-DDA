---
layout: page
title: ""
---

# Issues

## How to create new issues properly

GitHub issues are used for everything from bug reporting to long-term ideas. As such, you can make everything much smoother by following some simple rules.

### Rule zero

Always try to give your issue a meaningful title as this is the first thing anyone will see.

Note: `[CR]` and `[WIP]` "tags" are meaningful only for PRs. All open issues by definition are request for comments and work in progress.

### Bug reports

Before you submit a bug, always search the issues to see if it hasn't been reported already.

Your bug report should include:

- What OS did you experience the problem on (Windows, Linux, OS X etc.)
- What version were you playing:
 - Tiles or Curses (text-based)
 - Version string (preferably full version e.g. "0.C-4547-g3f1c109")
- A description of the problem you've found written in a way that enables anyone to try to recreate it.
- The behavior you expected from the game.

Your bug report may include:

- Screenshot(s) as some things are best explained visually
- A save file (e.g. link to a dropbox upload)

Bonus points for:

- Checking if the bug exists under the latest experimental build.
- Checking if it is OS specific.

The OS and CDDA version are very important - with the pace of changes here it is possible the bug you have encountered has already been fixed. After that reproducibility is the key, so write your report with all the necessary details.

### Enhancements and addition ideas

We have hundreds of issues open - most of them are ideas and suggestions. If you have a general idea or anything that can't be easily described in terms of *current* code changes you'd be better off suggesting it in appropriate section of the forum. You'll also get much broader exposure for your idea there. And getting a polished idea from the forum to GitHub issue should be a smooth move.

Otherwise please search first if maybe something like what you have on mind has been already proposed. If so feel free to join the discussion! If you think your idea is related but sufficiently different - open a new issue and perhaps refer to the older discussion (use GitHub's `#issue_number` reference system).

Remember to take part in the discussion of your suggestions.

### Questions

You should direct your question to the forum or ask on IRC. You should also checkout the included documentation and additional text files, e.g. `COMPILING.md` if you have problems building.

## Issue Triage

We have a fairly overwhelming number of open issues, 900+ as of this page's creation, we need to bring this under control, otherwise important issues can go unnoticed.  In order to make this happen, we need to step up issue triage, closing stale issues.

Ideally, issues will be found to be either invalid and closed quickly or found to be valid and addressed quickly.  Unfortunately there are some valid issues that cannot be addressed as quickly as we'd like, and they tend to accumulate.

### Reasons to close Issues
- Issue is invalid.  There are a number of reasons an issue might be invalid:
  - Bug report that can't be reproduced.  Regardless of the severity of a bug, if it can't be reproduced there's no value to keeping it around.  This doesn't mean we should close issues after just a few attempts to reproduce the issue, but if people have tried and failed to reproduce the issue, and there are no further instructions for reproduction, we can consider the report stale and close it.  These can be reopened if people find a reliable way to reproduce the issue.
  - Suggestion incompatible with direction of game development.  There are some suggestions that just don't fit in the game, and keeping them around adds no value.  Some examples of this kind of issue can be found [on the forums](https://discourse.cataclysmdda.org/t/fms-frequently-made-suggestions/), and it will be useful to tag such issues to create more examples.
  - Trivial suggestion.  Some issues are just to trivial to bother with tracking, for example requests to add a particular item, profession, building, etc, especially if the suggestion lacks detail can be closed, ideally with a polite message suggesting that Pull Requests are welcome (assuming it's a good idea).
- Issue is stale. Valid, reproducable bugs should stick around forever, but some feature requests and discussions lose their value if no one is championing them.
  - Issues tagged "Suggestion" that are idle for a week or more can be closed, discussion issues should be short-lived and spawn more concrete issues.  If you are closing a suggestion, consider writing up recommendations made in the issue in a new Issue and closing the original.
  - Overly-specific outlines of features that are obviously an individual developer's notes to themselves about something they want to implement.  These can be valuable in the short term, but if it's been months since the last activity on the issue, we can consider closing the issue.

## Bounties

We keep the development and direction of the game community-driven so placing a bounty *does not* necessarily mean that change will be incorporated into the main game. But it may be incorporated as a mod (or not). As such feel free to post bounties on what you like, but remember we don't do "bounty-driven" development. A good way of thinking about bounties is as encouragement for contributors to work on a particular issue and certainly not as "paying for features".

## Issue resolution

We do not assign people to issues. If you plan to work on a bug fix or a validated idea feel free to just comment about that. Actual PRs are of much greater value than any assignments. In general the first correct PR about something will be the PR that will get merged, but remember: we are using Git - you can collaborate with someone else easily by sending them patches or PRs against their PR branch.
