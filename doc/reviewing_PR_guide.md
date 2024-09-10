# Background & Introduction

As the CDDA project grows, it is becoming harder for the few volunteers in project management to keep constant tabs on what gets merged.  Over the years, we've found there are two possible ways this shakes out.  Whenever the lead devs are away for real-life reasons for a while, either we get a huge backlog of PRs, or we wind up with a few things getting merged that really shouldn't have been.  This leads to having to revert work by enthusiastic and well-meaning contributors, which we consider a very bad outcome.  This document aims to do two things to reduce this:

1. Develop a standardized PR review process and checklist that can be used for complex PRs, and
2. Through the above, offer an easier way to assess if a PR is ready to be merged.

This **is not required** for all PRs, and we don't expect that to change.  Rather, we hope that a standardized checklist would help contributors and collaborators know what to do to review and approve a complex PR, which in turn would make it much easier for people with merge permissions to go through and assess for merging.  Additionally, this checklist could be used as a guide for people making PRs to see if they think they're ready to go.  Please don't use this as an excuse to 'rules lawyer' the project; a PR could potentially break all of these points and still be mergeable.  These are *guidelines*.

## How to use this document

It's up to you!  You could have it open in a tab and refer to it, you could read this article and internalize it and not directly cite it, or you could copy and paste the checklist into your review and go through it item by item.  If you're not sure how to review a PR, that last option is a pretty safe bet, but this whole thing is not intended to be a hard-and-fast ruleset, it's an attempt to aid reviewers and to standardize the kind of reviews we get to contain the information most important to mergers.

# PR Review Checklist

Each item in this checklist will be explained further in this document.  Make sure you've read the whole thing at least once before starting.

- [ ] 1. PR focuses on a single area.
- [ ] 2. PR is less than ~300 lines of C++ or ~1000 lines of JSON.
- [ ] 3. PR does not use material from outside the project.
- [ ] 4. Changes are consistent with the description of the PR.
- [ ] 5. Solutions for problems are maintainable and extendable.
- [ ] 6. Game balance changes are sufficiently justified with evidence / approved by leadership.
- [ ] 7. Any failing CI tests have been checked, and are not related to the PR.

Remember, *these are suggestions for how to review a PR*, not the rules for PR inclusion.  A PR can break any of these and it may not be a problem, but in general if a reviewer notices that these aren't met, it's a good sign that a PR might need to be looked at by someone higher in the project.

## 1. Focused on a single area

PRs must focus on a single area of changes.  While it's tempting to "just fix this other thing while I'm in here", this makes a PR messy to read, and even worse, if you add an unexpected bug, makes that bug much harder to track down.  This is a very important recommendation, and it's tough to think of any real excuse for violating it, even though the temptation is very understandable (and some of our lead devs are occasionally guilty of falling into it).  Note that this doesn't mean that a PR can't reach into a lot of files, or change a lot of things at once.  Sometimes that is inevitable.  On top of that, we don't want to have a PR add code for features that aren't used, most of the time, so we do recommend a PR try to bring everything to a functional level and implement at least a bit of it all at once.

In general, we would prefer even "overhaul" PRs to break their changes into bite-sized chunks where possible.  A PR rewriting how monster attacks work, with associated JSON changes might, for example: first implement the new attack code but leave the legacy attack code intact, and only change attacks on classic zombies; then, a follow-up PR might change attacks in all other zombies; then, a follow-up PR might change attacks on other monsters; then, a final PR might remove the legacy attack code.  Doing it like this gives us time to track the changes and notice problems in between PRs, and lets us review *both* the code for the new attacks *and* the balance of how monsters' attacks are changing.  If this were all done in a single 3000-line PR, it would be nearly impossible to carefully comb through each changed monster and make sure their new attack stats are appropriate.

As with all the other guidelines, this does NOT mean you should immediately close your large PR and give up.  If you have a large PR and can't see any reasonable way to break it up, there's a good chance it's still fine; it just needs a higher-up person to review it.

## 2. Length restrictions

The length restrictions are just a suggestion, but an important one.  If a PR goes longer than this, there's a high chance it violates point 1.  If it does not violate point 1 and has good reason for being very long (eg. it's a simple automated change to a ton of documents), then this one might be skipped.  If the changes are very tidily broken into organized commits, and those commits are not squashed during merge, then it might also be safe to ignore this one.  Mapgen changes are also pretty much always exempt because maps are huge... but if you have a lot of other changes, it might be a good idea to do the maps in a separate PR first.

The reason for this guideline is similar to #1.  Very long PRs have a higher chance of introducing errors, and then those errors are hard to find.  Reviewers and bugfixers are human, and the longer the single block of changes, the harder it is to find the one that introduced the problem.

Once again though, because it's caused problems before, some changes *can't* be broken into more than one PR, and that's okay if it's sufficiently justified.  In particular, we don't like dead code, so if a PR is long because it implements a new system and also adds a map or a monster that uses the new system, that's usually preferable to adding the map/monster in a follow-up.

PRs that don't adhere to this are likely to take longer to merge because they're going to be very hard to review, but we will eventually get to them.

## 3. Outside sourced material

CDDA is licensed under CC-BY-SA 4.0.  Unless you're really, really certain you understand *exactly* how this works, then you should not approve any PR using **any** material from outside the project: not even things like names, obvious references, et cetera.  Don't accept someone else's explanation for how it works, don't assume something is too trivial to count, and don't assume the material is fair use.  Just flag it as not meeting this criterion.

Failing this guideline doesn't mean your PR is rejected, it just means we need to take a closer look.  The lead devs have been forced to become pretty expert at this and we'd like to ensure our rules are followed consistently, so if this is flagged, we'll come in and assess if it's okay.

## 4. Changes consistent with the description

While the PR description doesn't necessarily need to exhaustively outline every single line of code change, reading the PR description should clearly tell the reviewer what they're going to see in the changes.  There should not be any surprises in there.  If something was changed after the description was written, it needs to be updated.

I find it helpful to think: if I was coming back to this PR in a year trying to remember what was in it to see if it might be responsible for a bug, could I use the PR description to figure that out?  If a person coming later *has* to read the code changes to even know if they're in the right spot, there's a chance the description needs work.

## 5. Maintainable and extendible

This is obviously very much up to the reviewer.  "Janky" solutions to problems are solutions that try to work around a limitation of the codebase with funny tricks.  We see them often in JSON contributions, especially EOC things, but they are also very common in C++.  Sometimes they are allowable, but exploiting the code in strange ways has a high chance of breaking as soon as something else tries to rely on the code.  This becomes more important if the PR is changing something that other parts of the game are likely to rely on.

## 6. Balance changes have been justified

Like 4, this is a bit of a personal call.  Changing the durability of a few items, or the spawn frequency of a bunch of things, doesn't require extensive justification.  "No houses have toasters, so I added toasters to most houses" is a fine change that needs very little sourcing.  "There should be more medieval armour in the game region" is a major change to balance and requires a lot more of a source, or failing that, a clear "go ahead" from either a senior project lead (eg. Kevin, Zhilkinserg, KorGgent, Erk) or multiple experienced developers (green names on Discord); sometimes multiple collaborators would be sufficient.  This is particularly true if it touches on any of the previous "controversial" topics, like bows and solar power, or if it touches on sci-fi/fantasy content (cbms, mutations, portals, etc).

Another way to think of this guideline might be "Extraordinary claims require extraordinary evidence, and ordinary claims require ordinary evidence".  If you tell me there should be duct tape spawning in hardware stores, we won't need much of a source.  If you state that solar panels alone can give an electric car all the power it needs, we're gonna need some very good evidence of that since nobody is making solar cars.

## 7. Failing CI tests aren't related

We have many CI tests that run for each PR.  If you're going to give a PR a passing review, *you must have looked at these*.  We are all lazy (and human) too, we know it's tempting to ignore them, but please don't.

# Indicators to call for a second opinion

These aren't part of the review checklist, but they constitute observations that would justify getting more than one person with merge permissions to weigh in on the change.  **These don't mean your PR isn't getting merged**.  They're just things we've noticed are often associated with PRs that become problems later.

- PR creator is resistant to changes suggested to PR, particularly from collaborators and up.
- PRs that are part of a large, sweeping overhaul, without an issue posted or similar document discussing the design direction and plan for the overhaul.
- Changes justified by what "players" need or want, without deeper rationale (contrary to popular belief, improving player experience *is* desirable, but our experience is that just 'asking people' will produce wildly different results depending on which people you're talking to, and changes justified this way rarely have much statistical rigour).
- Changes that revert previously controversial adjustments with new math; this can be a hard one to know, but for example, if someone starts bumping up bow damage based on their research, the bar to merge is *very high*, as this is a topic that's been researched *to death* by now and it's highly unlikely - although not at all impossible - that a new contributor is aware of all the work done before.
- Changes to any political hot-button topics.  These are frequent troll bait.  Anything that touches on pride flags, trans/gay rights, immigration, and real-life political figures should be watched closely.
- Similarly, overly edgy content - especially stuff that relates to cannibalism, NPC servitude, etc should be monitored very closely.
- Topical humour, memes, pop-culture references, etc. are often added with the best of intentions but their expiry date generally runs out before the PR has even finished merging.  They should be taken with a large grain of salt, even when going into something like Crazy Cataclysm.

# The role of Discord and the Discord roles

We don't expect all contributors to be active on Discord, but the official Discord server (see [README.md](../README.md)) is the fastest and most reliable way to find out who to talk to if your PR is having problems, or if you are a reviewer trying to call attention to a PR or ask about these problems.  Especially in the #development channel, there are very few questions on these topics that will get you in trouble.

These guidelines refer to "lead devs", "developers", and "collaborators" as people whose opinions on a review are extra helpful. The lead dev team is indicated on Discord as people whose names are in gold at the top of the member list, under "senior devs". Most of the green names on Discord are "developers" and have a nearly equal level of say, especially in the parts of the code they work on themselves. The blue names on Discord are 'collaborators', and while a collaborator on their own isn't the final word on what PRs will be accepted or not, the collaborator role is used to indicate people who generally understand how the project works and can be trusted. People with purple names ("contributors") are fine to ask for and offer an opinion, but they don't have any particular extra authority on the project, so take their word with a grain of salt. Heck, even take the word of senior people with a bit of salt; only Kevin is the ultimate authority.

Regardless of Discord roles, anyone who wants to can and should offer reviews on others' PRs.  Contributors are welcome to use this guideline for their reviews.
