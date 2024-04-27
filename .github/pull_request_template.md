<!-- HOW TO USE: Under each "#### Heading" below, enter information relevant to your pull request.
Leave the headings unless they don't apply to your PR.

Please read carefully.
Once a pull request is submitted, automatic stylistic and consistency checks will be performed on the PR's changes.
The results can be either seen under the "Files changed" section of a PR or in the check's details.

Rules for suggested pull requests:
- If possible, limit yourself to small changes, 500 strings at max. Exceptions are adding or changing maps, and changes, that won't work unless they are done in a single run (even then there can be ways) - violating it puts a lot of unnecessary work on our merge team.
- Do not scope creep. If you make a pull request "Add new gun", please do not make anything more than adding the gun and following changes, like changing the stats of the gun, removing other guns from itemgroups or tweaking zombie horse stats - violating it makes future search and debugging stuff much harder, since PR name is not related to what is changed in the game. "Who the hell removed the quest item from drop in location X in PR, that adds a new plushie" - this may be a quote from a person who was affected by scope creep
- Do not make omnibus PRs. Meaning do not make a single PR, that fixes ten different, not related issues, at once, even if they are all one string - same as previously mentioned scope creep, it doesn't help to search the changes when debugging, despite all power of git blame tool

NOTE: Please grant permission for repository maintainers to edit your PR.  It is EXTREMELY common for PRs to be held up due to trivial changes being requested and the author being unavailable to make them. -->

#### Summary
Category "Brief description"
<!-- This section should consist of exactly one line, edit the one above.
1. Replace the word "Category" with one of these words: Features, Content, Interface, Mods, Balance, Bugfixes, Performance, Infrastructure, Build, I18N.
2. Replace the text inside the quotes with a brief description of your changes.
Or if you don't want a changelog entry, replace the whole line with just the word "None" (with no quotes).
Examples:
1. None
2. Features "In-game Armor sprite change"
3. Interface "Show crafting failure chances in the crafting interface"
4. Infrastructure "JSON-ize slot machines"
5. Bugfixes "Crafting GUI: show how much recipe makes for non-charge items"
For more on the meaning of each category, see:
https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/CHANGELOG_GUIDELINES.md
If approved and merged, your summary will be added to the project changelog:
https://github.com/CleverRaven/Cataclysm-DDA/blob/master/data/changelog.txt -->

#### Purpose of change

<!-- With a few sentences, describe your reasons for making this change.
If it relates to an existing issue, you can link it with a # followed by the GitHub issue number, like #1234.
When you submit a pull request that completely resolves an issue, use [Github's closing keywords](https://docs.github.com/en/get-started/writing-on-github/working-with-advanced-formatting/using-keywords-in-issues-and-pull-requests#linking-a-pull-request-to-an-issue)
to automatically close the issue once your pull request is merged.
If there is no related issue, explain here what issue, feature, or other concern you are addressing.  If this is a bugfix, include steps to reproduce the original bug, so your fix can be verified. -->

#### Describe the solution

<!-- How does the feature work, or how does this fix a bug?  The easier you make your solution to understand, the faster it can get merged. -->

#### Describe alternatives you've considered

<!-- Explain any alternative solutions, different approaches, or possibilities you've considered using to solve the same problem. -->

#### Testing

<!-- Describe what steps you took to test that this PR resolved the bug or added the feature, and what tests you performed to make sure it didn't cause any regressions.  Also, include testing suggestions for reviewers and maintainers. See TESTING_YOUR_CHANGES.md -->

#### Additional context

<!-- Add any other context (such as mock-ups, proof of concepts or screenshots) about the feature or bugfix here. -->


<!--README: Cataclysm: Dark Days Ahead is released under the Creative Commons Attribution ShareAlike 3.0 license.
The code and content of the game are free to use, modify, and redistribute for any purpose whatsoever.
By contributing to the project you agree to the terms of the license and that any contribution you make will also be covered by the same license.
See http://creativecommons.org/licenses/by-sa/3.0/ for details. -->
