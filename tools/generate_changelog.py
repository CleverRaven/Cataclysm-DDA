#!/usr/bin/env python3

import re
import urllib.request
import urllib.parse
import urllib.error
import json
import itertools
import logging
import threading
import collections
import time
import os
import sys
import argparse
import pathlib
import contextlib
import xml.etree.ElementTree
from datetime import date, datetime, timedelta


log = logging.getLogger('generate_changelog')


class MissingCommitException(Exception): pass


class JenkinsBuild:
    """Representation of a Jenkins Build"""

    def __init__(self, number, last_hash, branch, build_dttm, is_building, build_result, block_ms, wait_ms, build_ms):
        self.number = number
        self.last_hash = last_hash
        self.branch = branch
        self.build_dttm = build_dttm
        self.is_building = is_building
        self.build_result = build_result
        self.wait_ms = wait_ms
        self.build_ms = build_ms
        self.block_ms = block_ms

    def was_successful(self):
        return not self.is_building and self.build_result == 'SUCCESS'

    def __str__(self):
        return f'{self.__class__.__name__}[{self.number} - {self.last_hash} - {self.build_dttm} - {self.build_result}]'


class Commit:
    """Representation of a generic GitHub Commit"""

    def __init__(self, hash_id, message, commit_dttm, author, parents):
        self.hash = hash_id
        self.message = message
        self.commit_dttm = commit_dttm
        self.author = author
        self.parents = parents

    @property
    def commit_date(self):
        return self.commit_dttm.date()

    def committed_after(self, commit_dttm):
        return self.commit_dttm is not None and self.commit_dttm >= commit_dttm

    @property
    def is_merge(self):
        return len(self.parents) > 1

    def __str__(self):
        return f'{self.__class__.__name__}[{self.hash} - {self.commit_dttm} - {self.message} BY {self.author}]'


class PullRequest:
    """Representation of a generic GitHub Pull Request"""

    def __init__(self, pr_id, title, author, state, body, merge_hash, merge_dttm, update_dttm):
        self.id = pr_id
        self.author = author
        self.title = title
        self.body = body
        self.state = state
        self.merge_hash = merge_hash
        self.merge_dttm = merge_dttm
        self.update_dttm = update_dttm

    @property
    def update_date(self):
        return self.update_dttm.date()

    @property
    def merge_date(self):
        return self.merge_dttm.date()

    @property
    def is_merged(self):
        return self.merge_dttm is not None

    def merged_after(self, merge_dttm):
        return self.merge_dttm is not None and self.merge_dttm >= merge_dttm

    def updated_after(self, update_dttm):
        return self.update_dttm is not None and self.update_dttm >= update_dttm

    def __str__(self):
        return f'{self.__class__.__name__}[{self.id} - {self.merge_dttm} - {self.title} BY {self.author}]'


class SummaryType:
    """Different valid Summary Types. Intended to be used as a enum/constant class, no instantiation needed."""

    NONE = 'NONE'
    FEATURES = 'FEATURES'
    CONTENT = 'CONTENT'
    INTERFACE = 'INTERFACE'
    MODS = 'MODS'
    BALANCE = 'BALANCE'
    BUGFIXES = 'BUGFIXES'
    PERFORMANCE = 'PERFORMANCE'
    INFRASTRUCTURE = 'INFRASTRUCTURE'
    BUILD = 'BUILD'
    I18N = 'I18N'


class CDDAPullRequest(PullRequest):
    """A Pull Request with logic specific to CDDA Repository and their "Summary" descriptions"""

    SUMMARY_REGEX = re.compile( r'^`*(?i:SUMMARY):\s+(?P<pr_type>\w+)\s*(?:"(?P<pr_desc>.+)")?', re.MULTILINE )

    VALID_SUMMARY_CATEGORIES = (
        'Content',
        'Features',
        'Interface',
        'Mods',
        'Balance',
        'I18N',
        'Bugfixes',
        'Performance',
        'Build',
        'Infrastructure',
        'None',
    )

    EXAMPLE_SUMMARIES_IN_TEMPLATE = (
        ("Category", "description"),
        ("Content", "Adds new mutation category 'Mouse'"),
    )

    def __init__(self, pr_id, title, author, state, body, merge_hash, merge_dttm, update_dttm, store_body=False):
        super().__init__(pr_id, title, author, state, body if store_body else '', merge_hash, merge_dttm, update_dttm)
        self.summ_type, self.summ_desc = self._get_summary(body)

    @property
    def summ_type(self):
        return self._summ_type

    @summ_type.setter
    def summ_type(self, value):
        self._summ_type = value.upper() if value is not None else None

    @property
    def has_valid_summary(self):
        return self.summ_type == SummaryType.NONE or (self.summ_type is not None and self.summ_desc is not None)

    def _get_summary(self, body):
        matches = list(re.finditer(self.SUMMARY_REGEX, body))
        ### fix weird cases where a PR have multiple SUMMARY
        ### coming mostly from template lines that weren't removed by the pull requester
        ### example: https://api.github.com/repos/CleverRaven/Cataclysm-DDA/pulls/25604
        summary_filter = lambda x: (x.group('pr_type'), x.group('pr_desc')) not in self.EXAMPLE_SUMMARIES_IN_TEMPLATE
        matches = list(filter(summary_filter, matches))
        if len(matches) > 1:
            log.warning(f'More than one SUMMARY defined in PR {self.id}!')

        match = matches[0] if matches else None
        if match is None or match.group('pr_type').upper() not in (x.upper() for x in self.VALID_SUMMARY_CATEGORIES):
            return None, None
        else:
            return match.group('pr_type'), match.group('pr_desc')

    def __str__(self):
        if self.has_valid_summary and self.summ_type == SummaryType.NONE:
            return (f'{self.__class__.__name__}'
                    f'[{self.id} - {self.merge_dttm} - {self.summ_type} - {self.title} BY {self.author}]')
        elif self.has_valid_summary:
            return (f'{self.__class__.__name__}'
                    f'[{self.id} - {self.merge_dttm} - {self.summ_type} - {self.summ_desc} BY {self.author}]')
        else:
            return (f'{self.__class__.__name__}'
                    f'[{self.id} - {self.merge_dttm} - {self.title} BY {self.author}]')


class JenkinsBuildFactory:
    """Abstraction for instantiation of new Commit objects"""

    def create(self, number, last_hash, branch, build_dttm, is_building, build_result, block_ms, wait_ms, build_ms):
        return JenkinsBuild(number, last_hash, branch, build_dttm, is_building,
                            build_result, block_ms, wait_ms, build_ms)

class CommitFactory:
    """Abstraction for instantiation of new Commit objects"""

    def create(self, hash_id, message, commit_date, author, parents):
        return Commit(hash_id, message, commit_date, author, parents)


class CDDAPullRequestFactory:
    """Abstraction for instantiation of new CDDAPullRequests objects"""

    def __init__(self, store_body=False):
        self.store_body = store_body

    def create(self, pr_id, title, author, state, body, merge_hash, merge_dttm, update_dttm):
        return CDDAPullRequest(pr_id, title, author, state, body, merge_hash, merge_dttm, update_dttm, self.store_body)


class CommitRepository:
    """Groups Commits for storage and common operations"""

    def __init__(self):
        self.ref_by_commit_hash = {}
        self._latest_commit = None

    def add(self, commit):
        if self._latest_commit is None or self._latest_commit.commit_date < commit.commit_date:
            self._latest_commit = commit
        self.ref_by_commit_hash[commit.hash] = commit

    def add_multiple(self, commits):
        for commit in commits:
            self.add(commit)

    def get_commit(self, commit_hash):
        return self.ref_by_commit_hash[commit_hash] if commit_hash in self.ref_by_commit_hash else None

    def get_latest_commit(self):
        return self._latest_commit

    def get_all_commits(self, filter_by=None, sort_by=None):
        """Return all Commits in Repository. No order is guaranteed."""
        commit_list = self.ref_by_commit_hash.values()

        if filter_by is not None:
            commit_list = filter(filter_by, commit_list)

        if sort_by is not None:
            commit_list = sorted(commit_list, key=sort_by)

        for commit in commit_list:
            yield commit

    def traverse_commits_by_first_parent(self, initial_hash=None):
        """Iterate through Commits connected by the first Parent, until a parent is not found in the Repository

        This is like using 'git log --first-parent $commit', which avoids Commits "inside" Pull Requests / Merges.
        But returns Merge Commits (which often can be related to a single PR) and Commits directly added to a Branch.
        """
        if initial_hash is not None:
            commit = self.get_commit(initial_hash)
        else:
            commit = self.get_latest_commit()

        while commit is not None:
            yield commit
            commit = self.get_commit(commit.parents[0])

    def get_commit_range_by_hash(self, latest_hash, oldest_hash):
        """Return Commits between initial_hash (including) and oldest_hash (excluding) connected by the first Parent."""
        for commit in self.traverse_commits_by_first_parent(latest_hash):
            if commit.hash == oldest_hash:
                return
            yield commit
        ### consumed the whole commit list in the Repo and didn't find oldest_hash
        ### so returned commit list is incomplete
        raise MissingCommitException(
            "Can't generate commit list for specified hash range."
            " There are missing Commits in CommitRepository."
        )

    def get_commit_range_by_date(self, latest_dttm, oldest_dttm):
        """Return Commits between latest_dttm (including) and oldest_dttm (excluding) connected by the first Parent."""
        for commit in self.traverse_commits_by_first_parent():
            ### assuming that traversing by first parent have chronological order (should be DESC BY commit_dttm)
            if commit.commit_dttm <= oldest_dttm:
                return
            if latest_dttm >= commit.commit_dttm > oldest_dttm:
                yield commit
        ### consumed the whole commit list and didn't find a commit past our build.
        ### so returned commit list *could be* incomplete
        raise MissingCommitException(
            "Can't generate commit list for specified date range."
            " There are missing Commits in CommitRepository."
        )

    def purge_references(self):
        self.ref_by_commit_hash.clear()


class CDDAPullRequestRepository:
    """Groups Pull Requests for storage and common operations"""

    def __init__(self):
        self.ref_by_pr_id = {}
        self.ref_by_merge_hash = {}

    def add(self, pull_request):
        self.ref_by_pr_id[pull_request.id] = pull_request
        if pull_request.merge_hash is not None:
            self.ref_by_merge_hash[pull_request.merge_hash] = pull_request

    def add_multiple(self, pull_requests):
        for pr in pull_requests:
            self.add(pr)

    def get_pr_by_merge_hash(self, merge_hash):
        return self.ref_by_merge_hash[merge_hash] if merge_hash in self.ref_by_merge_hash else None

    def get_all_pr(self, filter_by=None, sort_by=None):
        """Return all Pull Requests in Repository. No order is guaranteed."""
        pr_list = self.ref_by_pr_id.values()

        if filter_by is not None:
            pr_list = filter(filter_by, pr_list)

        if sort_by is not None:
            pr_list = sorted(pr_list, key=sort_by)

        for pr in pr_list:
            yield pr

    def get_merged_pr_list_by_date(self, latest_dttm, oldest_dttm):
        """Return PullRequests merged between latest_dttm (including) and oldest_dttm (excluding)."""
        for pr in self.get_all_pr(filter_by=lambda x: x.is_merged, sort_by=lambda x: -x.merge_dttm.timestamp()):
            if latest_dttm >= pr.merge_dttm > oldest_dttm:
                yield pr

    def purge_references(self):
        self.ref_by_merge_hash.clear()


class JenkinsBuildRepository:
    """Groups JenkinsBuilds for storage and common operations"""

    def __init__(self):
        self.ref_by_build_number = {}

    def add(self, build):
        if build.number is not None:
            self.ref_by_build_number[build.number] = build

    def add_multiple(self, builds):
        for build in builds:
            self.add(build)

    def get_build_by_number(self, build_number):
        return self.ref_by_build_number[build_number] if build_number in self.ref_by_build_number else None

    def get_previous_build(self, build_number, condition=lambda x: True):
        for x in range(build_number - 1, 0, -1):
            prev_build = self.get_build_by_number(x)
            if prev_build is not None and condition(prev_build):
                return prev_build
        return None

    def get_previous_successful_build(self, build_number):
        return self.get_previous_build(build_number, lambda x: x.was_successful())

    def get_all_builds(self, filter_by=None, sort_by=None):
        """Return all Builds in Repository. No order is guaranteed."""
        build_list = self.ref_by_build_number.values()

        if filter_by is not None:
            build_list = filter(filter_by, build_list)

        if sort_by is not None:
            build_list = sorted(build_list, key=sort_by)

        for build in build_list:
            yield build

    def purge_references(self):
        self.ref_by_build_number.clear()


class JenkinsApi:

    JENKINS_BUILD_LIST_API = r'http://gorgon.narc.ro:8080/job/Cataclysm-Matrix/api/xml'

    JENKINS_BUILD_LONG_LIST_PARAMS = {
        'tree': r'allBuilds[number,timestamp,building,result,actions[buildingDurationMillis,waitingDurationMillis,blockedDurationMillis,lastBuiltRevision[branch[name,SHA1]]]]',
        'xpath': r'//allBuild',
        'wrapper': r'allBuilds',
        'exclude': r'//action[not(buildingDurationMillis|waitingDurationMillis|blockedDurationMillis|lastBuiltRevision/branch)]'
    }

    JENKINS_BUILD_SHORT_LIST_PARAMS = {
        'tree': r'builds[number,timestamp,building,result,actions[buildingDurationMillis,waitingDurationMillis,blockedDurationMillis,lastBuiltRevision[branch[name,SHA1]]]]',
        'xpath': r'//build',
        'wrapper': r'builds',
        'exclude': r'//action[not(buildingDurationMillis|waitingDurationMillis|blockedDurationMillis|lastBuiltRevision/branch)]'
    }

    def __init__(self, build_factory):
        self.build_factory = build_factory

    def get_build_list(self):
        """Return the builds from Jenkins. API limits the result to last 999 builds."""
        request_url = self.JENKINS_BUILD_LIST_API + '?' + urllib.parse.urlencode(self.JENKINS_BUILD_LONG_LIST_PARAMS)
        api_request = urllib.request.Request(request_url)
        log.debug(f'Processing Request {api_request.full_url}')
        with urllib.request.urlopen(api_request) as api_response:
            api_data = xml.etree.ElementTree.fromstring(api_response.read())
        log.debug('Jenkins Request DONE!')

        for build_data in api_data:
            yield self._create_build_from_api_data(build_data)

    def _create_build_from_api_data(self, build_data):
        """Create a JenkinsBuild instance based on data from Jenkins API"""
        jb_number = int(build_data.find('number').text)
        jb_build_dttm = datetime.utcfromtimestamp(int(build_data.find('timestamp').text) // 1000)
        jb_is_building = build_data.find('building').text == 'true'

        jb_block_ms = timedelta(0)
        jb_wait_ms = timedelta(0)
        jb_build_ms = timedelta(0)
        jb_build_result = None
        if not jb_is_building:
            jb_build_result = build_data.find('result').text
            jb_block_ms = timedelta(milliseconds=int(build_data.find(r'.//action/blockedDurationMillis').text))
            jb_wait_ms = timedelta(milliseconds=int(build_data.find(r'.//action/waitingDurationMillis').text))
            jb_build_ms = timedelta(milliseconds=int(build_data.find(r'.//action/buildingDurationMillis').text))

        jb_last_hash = None
        jb_branch = None
        if build_data.find(r'.//action/lastBuiltRevision/branch/SHA1') is not None:
            jb_last_hash = build_data.find(r'.//action/lastBuiltRevision/branch/SHA1').text
            jb_branch = build_data.find(r'.//action/lastBuiltRevision/branch/name').text

        return self.build_factory.create(jb_number, jb_last_hash, jb_branch, jb_build_dttm, jb_is_building,
                                         jb_build_result, jb_block_ms, jb_wait_ms, jb_build_ms)


class CommitApi:

    def __init__(self, commit_factory, api_token):
        self.commit_factory = commit_factory
        self.api_token = api_token

    def get_commit_list(self, min_commit_dttm, max_commit_dttm, branch='master', max_threads=15):
        """Return a list of Commits from specified commit date up to now. Order is not guaranteed by threads.

            params:
                min_commit_dttm = None or minimum commit date to be part of the result set (UTC+0 timezone)
            params:
                max_commit_dttm = None or maximum commit date to be part of the result set
        """
        results_queue = collections.deque()
        request_generator = CommitApiGenerator(self.api_token, min_commit_dttm, max_commit_dttm, branch)
        threaded = MultiThreadedGitHubApi()
        threaded.process_api_requests(request_generator,
                                      self._process_commit_data_callback(results_queue),
                                      max_threads=max_threads)

        return (commit for commit in results_queue if commit.committed_after(min_commit_dttm))

    def _process_commit_data_callback(self, results_queue):
        """Returns a callback that will process data into Commits instances and stop threads when needed."""
        def _process_commit_data_callback_closure(json_data, request_generator):
            nonlocal results_queue
            commit_list = [self._create_commit_from_api_data(x) for x in json_data]
            for commit in commit_list:
                results_queue.append(commit)

            if request_generator.is_active and len(commit_list) == 0:
                log.debug(f'Target page found, stop giving threads more requests')
                request_generator.deactivate()
        return _process_commit_data_callback_closure

    def _create_commit_from_api_data(self, commit_data):
        """Create a Commit instance based on data from GitHub API"""
        if commit_data is None:
            return None
        commit_sha = commit_data['sha']
        ### some commits have null in author.login :S like:
        ### https://api.github.com/repos/CleverRaven/Cataclysm-DDA/commits/569bef1891a843ec71654530a64d51939aabb3e2
        ### I try to use author.login when possible or fallback to "commit.author" which doesn't match
        ### with usernames in pull requests API (I guess it comes from the distinction between "name" and "username".
        ### I rather have a name that doesn't match that leave it empty.
        ### Anyways, I'm surprised but GitHub API sucks, is super inconsistent and not well thought or documented.
        if commit_data['author'] is not None:
            commit_author = commit_data['author']['login']
        else:
            commit_author = commit_data['commit']['author']['name']
        commit_message = commit_data['commit']['message'].splitlines()[0] if commit_data['commit']['message'] else ''
        commit_dttm = commit_data['commit']['committer']['date']
        commit_dttm = datetime.fromisoformat(commit_dttm.rstrip('Z')) if commit_dttm else None
        commit_parents = tuple(p['sha'] for p in commit_data['parents'])

        return self.commit_factory.create(commit_sha, commit_message, commit_dttm, commit_author, commit_parents)


class PullRequestApi:

    GITHUB_API_SEARCH = r'https://api.github.com/search/issues'

    def __init__(self, pr_factory, api_token):
        self.pr_factory = pr_factory
        self.api_token = api_token

    def search_for_pull_request(self, commit_hash):
        """Returns the Pull Request ID of a specific Commit Hash using a Search API in GitHub

        AVOID AT ALL COST: You have to check Commit by Commit and is super slow!

        Based on https://help.github.com/articles/searching-issues-and-pull-requests/#search-by-commit-sha
        No need to make this multi-threaded as GitHub limits this to 30 requests per minute.
        """
        params = {'q': f'is:pr is:merged repo:CleverRaven/Cataclysm-DDA {commit_hash}'}
        request_builder = GitHubApiRequestBuilder(self.api_token)
        api_request = request_builder.create_request(self.GITHUB_API_SEARCH, params)

        api_data = do_github_request(api_request)

        ### I found some cases where the search found multiple PRs because of a reference of the commit hash
        ### in another PR description. But it had a huge difference in score, it seems the engine scores
        ### over 100 the one that actually has the commit hash as "Merge Commit SHA".
        ### Output example:
        ### https://api.github.com/search/issues
        ### ?q=is%3Apr+is%3Amerged+repo%3ACleverRaven%2FCataclysm-DDA+f0c6908d154cd0fb190c2116de2bf2d3131458c3

        ### If the API don't return a score of 100 or more for the highest score item (the first item),
        ### then the commit is probably not part of any pull request
        ### This assumption was backed up by checking ~9 months of commits
        if api_data['total_count'] == 0 or api_data['items'][0]['score'] < 100:
            return None
        else:
            return api_data['items'][0]['number']

    def get_pr_list(self, min_dttm, max_dttm, state='all', merged_only=False, max_threads=15):
        """Return a list of PullRequests from specified commit date up to now. Order is not guaranteed by threads.

            params:
                min_dttm = None or minimum update date on the PR to be part of the result set (UTC+0 timezone)
                min_dttm = None or maximum update date on the PR to be part of the result set (UTC+0 timezone)
                state = 'open' | 'closed' | 'all'
                merged_only = search only 'closed' state PRs, and filter PRs by merge date instead of update date
        """
        results_queue = collections.deque()
        request_generator = PullRequestApiGenerator(self.api_token, state if not merged_only else 'closed')
        threaded = MultiThreadedGitHubApi()
        threaded.process_api_requests(request_generator,
                                      self._process_pr_data_callback(results_queue, min_dttm, max_dttm, merged_only),
                                      max_threads=max_threads)

        if merged_only:
            return (pr for pr in results_queue if pr.is_merged and pr.merged_after(min_dttm))
        else:
            return (pr for pr in results_queue if pr.updated_after(min_dttm))

    def _process_pr_data_callback(self, results_queue, min_dttm, max_dttm, merged_only):
        """Returns a callback that will process data into Pull Requests objects and stop threads when needed."""
        def _process_pr_data_callback_closure(json_data, request_generator):
            nonlocal results_queue, min_dttm, merged_only
            pull_request_list = [self._create_pr_from_api_data(x) for x in json_data]
            for pr in pull_request_list:
                if not merged_only or pr.is_merged:
                    results_queue.append(pr)

            target_page_found = False
            if min_dttm is not None and merged_only:
                target_page_found = not any(pr.merged_after(min_dttm) for pr in pull_request_list)
            if min_dttm is not None and not merged_only:
                target_page_found = not any(pr.updated_after(min_dttm) for pr in pull_request_list)

            if len(pull_request_list) == 0 or target_page_found:
                if request_generator.is_active:
                    log.debug(f'Target page found, stop giving threads more requests')
                    request_generator.deactivate()

        return _process_pr_data_callback_closure

    def _create_pr_from_api_data(self, pr_data):
        """Create a PullRequest instance based on data from GitHub API"""
        if pr_data is None:
            return None
        pr_number = pr_data['number']
        pr_author = pr_data['user']['login']
        pr_title = pr_data['title']
        pr_state = pr_data['state']
        pr_merge_hash = pr_data['merge_commit_sha']
        ### python expects an ISO date with the Z to properly parse it, so I strip it.
        pr_merge_dttm = datetime.fromisoformat(pr_data['merged_at'].rstrip('Z')) if pr_data['merged_at'] else None
        pr_update_dttm = datetime.fromisoformat(pr_data['updated_at'].rstrip('Z')) if pr_data['updated_at'] else None
        ### PR description can be empty :S example: https://github.com/CleverRaven/Cataclysm-DDA/pull/24213
        pr_body = pr_data['body'] if pr_data['body'] else ''

        return self.pr_factory.create(pr_number, pr_title, pr_author, pr_state, pr_body,
                                      pr_merge_hash, pr_merge_dttm, pr_update_dttm)


class MultiThreadedGitHubApi:

    def process_api_requests(self, request_generator, callback, max_threads=15):
        """Process HTTP API requests on threads and call the callback for each result JSON

            params:
                callback = executed when data is available, should expect two params: (json data, request_generator)
        """
        ### start threads
        threads = []
        for x in range(1, max_threads + 1):
            t = threading.Thread(target=exit_on_exception(self._process_api_requests_on_threads),
                                 args=(request_generator, callback))
            t.name = f'WORKER_{x:03}'
            threads.append(t)
            t.daemon = True
            t.start()
            time.sleep(0.1)

        ### block waiting until threads get all results
        for t in threads:
            t.join()
        log.debug('Threads have finished processing all the required GitHub API Requests!!!')

    @staticmethod
    def _process_api_requests_on_threads(request_generator, callback):
        """Process HTTP API requests and call the callback for each result JSON"""
        log.debug(f'Thread Started!')
        api_request = request_generator.generate()
        while api_request is not None:
            callback(do_github_request(api_request), request_generator)
            api_request = request_generator.generate()
        log.debug(f'No more requests left, killing Thread.')


class GitHubApiRequestBuilder:

    def __init__(self, api_token, timezone='Etc/UTC'):
        self.api_token = api_token
        self.timezone = timezone

    def create_request(self, url, params=None):
        """Creates an API request based on provided URL and GET Parameters"""
        if params is None:
            request_url = url
        else:
            request_url = url + '?' + urllib.parse.urlencode(params)

        if self.api_token is None:
            api_request = urllib.request.Request(request_url)
        else:
            api_headers = {
                'Authorization': 'token ' + self.api_token,
                'Time-Zone': self.timezone,
            }
            api_request = urllib.request.Request(request_url, headers=api_headers)

        return api_request


class CommitApiGenerator(GitHubApiRequestBuilder):
    """Generates multiple HTTP requests to get Commits, used from Threads to get data until a condition is met."""

    GITHUB_API_LIST_COMMITS = r'https://api.github.com/repos/CleverRaven/Cataclysm-DDA/commits'

    def __init__(self, api_token, since_dttm=None, until_dttm=None, sha='master',
                 initial_page=1, step=1, timezone='Etc/UTC'):
        super().__init__(api_token, timezone)
        self.sha = sha
        self.since_dttm = since_dttm
        self.until_dttm = until_dttm
        self.page = initial_page
        self.step = step
        self.lock = threading.RLock()
        self.is_active = True

    @property
    def is_active(self):
        with self.lock:
            return self._is_active

    @is_active.setter
    def is_active(self, value):
        with self.lock:
            self._is_active = value

    def deactivate(self):
        """Stop generate() from creating new HTTP requests on future calls."""
        self.is_active = False

    def generate(self):
        """Returns an HTTP request to get Commits for a different API result page each call until deactivate()."""
        with self.lock:
            if self.is_active:
                req = self.create_request(self.since_dttm, self.until_dttm, self.sha, self.page)
                self.page += self.step
                return req
            else:
                return None

    def create_request(self, since_dttm=None, until_dttm=None, sha='master', page=1):
        """Creates an HTTP Request to GitHub API to get Commits from CDDA repository."""
        params = {
            'sha': sha,
            'page': page,
        }
        if since_dttm is not None:
            params['since'] = since_dttm.isoformat()
        if until_dttm is not None:
            params['until'] = until_dttm.isoformat()

        return super().create_request(self.GITHUB_API_LIST_COMMITS, params)


class PullRequestApiGenerator(GitHubApiRequestBuilder):
    """Generates multiple HTTP requests to get Pull Requests, used from Threads to get data until a condition is met."""

    GITHUB_API_LIST_PR = r'https://api.github.com/repos/CleverRaven/Cataclysm-DDA/pulls'

    def __init__(self, api_token, state='all', initial_page=1, step=1, timezone='Etc/UTC'):
        super().__init__(api_token, timezone)
        self.page = initial_page
        self.step = step
        self.state = state
        self.lock = threading.RLock()
        self.is_active = True

    @property
    def is_active(self):
        with self.lock:
            return self._is_active

    @is_active.setter
    def is_active(self, value):
        with self.lock:
            self._is_active = value

    def deactivate(self):
        """Stop generate() from creating new HTTP requests on future calls."""
        self.is_active = False

    def generate(self):
        """Returns an HTTP request to get Pull Requests for a different API result page each call until deactivate()."""
        with self.lock:
            if self.is_active:
                req = self.create_request(self.state, self.page)
                self.page += self.step
                return req
            else:
                return None

    def create_request(self, state='all', page=1):
        """Creates an HTTP Request to GitHub API to get Pull Requests from CDDA repository.

            params:
                state = 'open' | 'closed' | 'all'
        """
        params = {
            'state': state,
            'sort': 'updated',
            'direction': 'desc',
            'page': page,
        }
        return super().create_request(self.GITHUB_API_LIST_PR, params)


def exit_on_exception(func):
    """Decorator to terminate the main script and all threads if a thread generates an Exception"""
    def exit_on_exception_closure(*args, **kwargs):
        try:
            func(*args, **kwargs)
        except Exception as err:
            log.exception(f'Unhandled Exception: {err}')
            os._exit(-10)
    return exit_on_exception_closure


def do_github_request(api_request, retry_on_limit=3):
    """Do an HTTP request to GitHub and retries in case of hitting API limits"""
    for retry in range(1, retry_on_limit + 2):
        try:
            log.debug(f'Processing Request {api_request.full_url}')
            with urllib.request.urlopen(api_request) as api_response:
                return json.load(api_response)
        except urllib.error.HTTPError as err:
            ### hit rate limit, wait and retry
            if err.code == 403 and err.getheader('Retry-After'):
                wait = int(err.getheader('Retry-After')) + 5
                log.info(f'Reached GitHub API rate limit. Retry {retry}, waiting {wait} secs...')
                time.sleep(wait)
            elif err.code == 403 and err.getheader('X-RateLimit-Remaining') == '0':
                delta = datetime.utcfromtimestamp(int(err.getheader('X-RateLimit-Reset'))) - datetime.utcnow()
                wait = delta.seconds + 5
                log.info(f'Reached GitHub API rate limit. Retry {retry}, waiting {wait} secs...')
                time.sleep(wait)
            else:
                ### other kind of http error, just implode
                log.exception(f'Unhandled Exception: {err} - HTTP Headers: {err.getheaders()}')
                raise
    raise Exception(f'Retry limit reached')


def read_personal_token(filename):
    """Return Personal Token from specified file, None if no file is provided or file doesn't exist.

    Personal Tokens can be generated in https://github.com/settings/tokens
    This makes GitHub API have higher usage limits
    """
    if filename is None:
        return None

    try:
        with open(pathlib.Path(str(filename)).expanduser()) as token_file:
            match = re.search('(?P<token>\S+)', token_file.read(), flags=re.MULTILINE)
            if match is not None:
                return match.group('token')
    except IOError:
        pass

    return None


@contextlib.contextmanager
def smart_open(filename=None, *args, **kwargs):
    if filename and (filename == '-' or filename == sys.stdout):
        fh = sys.stdout
    else:
        fh = open(filename, *args, **kwargs)

    try:
        yield fh
    finally:
        if fh is not sys.stdout:
            fh.close()


def validate_file_for_writing(filepath):
    if (filepath is not None and
        filepath != sys.stdout and
            (not filepath.parent.exists()
             or not filepath.parent.is_dir())):
        return False


def main_entry(argv):
    parser = argparse.ArgumentParser(description='''Generates Changelog from now until the specified data.\ngenerate_changelog.py -D changelog_2019_03 -t ../repo_token -f -e 2019-04-01 2019-03-01''', formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        'target_date',
        help='Specify when should stop generating. Accepts "YYYY-MM-DD" ISO 8601 Date format.',
        type=lambda d: datetime.combine(date.fromisoformat(d), datetime.min.time())
    )

    parser.add_argument(
        '-D', '--by-date',
        help='Indicates changes should be presented grouped by Date. Requires a filename parameter, "-" for STDOUT',
        type=lambda x: sys.stdout if x == '-' else pathlib.Path(x).expanduser().resolve(),
        default=None
    )

    parser.add_argument(
        '-B', '--by-build',
        help='Indicates changes should be presented grouped by Build. Requires a filename parameter, "-" for STDOUT',
        type=lambda x: sys.stdout if x == '-' else pathlib.Path(x).expanduser().resolve(),
        default=None
    )

    parser.add_argument(
        '-e', '--end-date',
        help='Specify when should start generating. Accepts "YYYY-MM-DD" ISO 8601 Date format.',
        type=lambda d: datetime.combine(date.fromisoformat(d), datetime.min.time()),
        default=None
    )

    parser.add_argument(
        '-t', '--token-file',
        help='Specify where to read the Personal Token. Default "~/.generate_changelog.token".',
        type=lambda x: pathlib.Path(x).expanduser().resolve(),
        default='~/.generate_changelog.token'
    )

    parser.add_argument(
        '-N', '--include-summary-none',
        action='store_true',
        help='Indicates if Pull Requests with Summary "None" should be included in the output.',
        default=None
    )

    parser.add_argument(
        '-f', '--flatten-output',
        action='store_true',
        help='Output a flattened format with no headings.',
        default=None
    )

    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Indicates the logging system to generate more information about actions.',
        default=None
    )

    arguments = parser.parse_args(argv[1:])

    if (arguments.end_date is None):
        arguments.end_date=datetime.now()

    logging.basicConfig(level=logging.DEBUG if arguments.verbose else logging.INFO,
                        format='   LOG | %(threadName)s | %(levelname)s | %(message)s')

    log.debug(f'Commandline Arguments (+defaults): {arguments}')

    if validate_file_for_writing(arguments.by_date):
        raise ValueError(f"Specified directory in --by-date doesn't exist: {arguments.by_date.parent}")

    if validate_file_for_writing(arguments.by_build):
        raise ValueError(f"Specified directory in --by-build doesn't exist: {arguments.by_build.parent}")

    personal_token = read_personal_token(arguments.token_file)
    if personal_token is None:
        log.warning("GitHub Token was not provided, API calls will have severely limited rates.")

    if arguments.by_date is None and arguments.by_build is None:
        raise ValueError("Script should be called with either --by-date or --by-build arguments or both.")

    main_output(arguments.by_date, arguments.by_build, arguments.target_date, arguments.end_date,
                personal_token, arguments.include_summary_none, arguments.flatten_output)


def get_github_api_data(pr_repo, commit_repo, target_dttm, end_dttm, personal_token):

    def load_github_repos():
        commit_api = CommitApi(CommitFactory(), personal_token)
        commit_repo.add_multiple(commit_api.get_commit_list(target_dttm, end_dttm))

        pr_api = PullRequestApi(CDDAPullRequestFactory(), personal_token)
        pr_repo.add_multiple(pr_api.get_pr_list(target_dttm, end_dttm, merged_only=True))

    github_thread = threading.Thread(target=exit_on_exception(load_github_repos))
    github_thread.name = 'WORKER_GIT'
    github_thread.daemon = True
    github_thread.start()

    return github_thread


def get_jenkins_api_data(build_repo):

    def load_jenkins_repo():
        jenkins_api = JenkinsApi(JenkinsBuildFactory())
        build_repo.add_multiple(jenkins_api.get_build_list())

    jenkins_thread = threading.Thread(target=exit_on_exception(load_jenkins_repo))
    jenkins_thread.name = 'WORKER_JEN'
    jenkins_thread.daemon = True
    jenkins_thread.start()

    return jenkins_thread


def main_output(by_date, by_build, target_dttm, end_dttm, personal_token, include_summary_none, flatten):
    threads = []

    if by_build is not None:
        build_repo = JenkinsBuildRepository()
        threads.append(get_jenkins_api_data(build_repo))

    pr_repo = CDDAPullRequestRepository()
    commit_repo = CommitRepository()
    threads.append(get_github_api_data(pr_repo, commit_repo, target_dttm, end_dttm, personal_token))

    for thread in threads:
        thread.join()

    if by_date is not None:
        with smart_open(by_date, 'w', encoding='utf8') as output_file:
            build_output_by_date(pr_repo, commit_repo, target_dttm, end_dttm,
                                 output_file, include_summary_none, flatten)

    if by_build is not None:
        with smart_open(by_build, 'w', encoding='utf8') as output_file:
            build_output_by_build(build_repo, pr_repo, commit_repo, output_file, include_summary_none)


def build_output_by_date(pr_repo, commit_repo, target_dttm, end_dttm, output_file,
                         include_summary_none, flatten):
    ### group commits with no PR by date
    commits_with_no_pr = collections.defaultdict(list)
    for commit in commit_repo.traverse_commits_by_first_parent():
        if not pr_repo.get_pr_by_merge_hash(commit.hash):
            commits_with_no_pr[commit.commit_date].append(commit)

    ### group PRs by date
    pr_with_summary = collections.defaultdict(list)
    pr_with_invalid_summary = collections.defaultdict(list)
    pr_with_summary_none = collections.defaultdict(list)
    for pr in pr_repo.get_merged_pr_list_by_date(end_dttm, target_dttm):
        if pr.has_valid_summary and pr.summ_type == SummaryType.NONE:
            pr_with_summary_none[pr.merge_date].append(pr)
        elif pr.has_valid_summary:
            pr_with_summary[pr.merge_date].append(pr)
        elif not pr.has_valid_summary:
            pr_with_invalid_summary[pr.merge_date].append(pr)

    ### build main changelog output
    for curr_date in (end_dttm.date() - timedelta(days=x) for x in itertools.count()):
        if curr_date < target_dttm.date():
            break
        if curr_date not in pr_with_summary and curr_date not in commits_with_no_pr:
            continue

        print(f"{curr_date}", file=output_file, end='\n\n')
        sort_by_type = lambda pr: pr.summ_type
        sorted_pr_list_by_category = sorted(pr_with_summary[curr_date], key=sort_by_type)
        for pr_type, pr_list_by_category in itertools.groupby(sorted_pr_list_by_category, key=sort_by_type):
            if not flatten:
                print(f"    {pr_type}", file=output_file)
            for pr in pr_list_by_category:
                if flatten:
                    print(f"{pr_type} {pr.summ_desc}", file=output_file)
                else:
                    print(f"        * {pr.summ_desc} (by {pr.author} in PR {pr.id})", file=output_file)
            print(file=output_file)

        if curr_date in commits_with_no_pr:
            if not flatten:
                print(f"    MISC. COMMITS", file=output_file)
            for commit in commits_with_no_pr[curr_date]:
                if not flatten:
                    print(f"        * {commit.message} (by {commit.author} in Commit {commit.hash[:7]})",
                          file=output_file)
                else:
                    print(f"COMMIT {commit.message})", file=output_file)
            print(file=output_file)

        if curr_date in pr_with_invalid_summary or (include_summary_none and curr_date in pr_with_summary_none):
            if not flatten:
                print(f"    MISC. PULL REQUESTS", file=output_file)
            for pr in pr_with_invalid_summary[curr_date]:
                if not flatten:
                    print(f"        * {pr.title} (by {pr.author} in PR {pr.id})", file=output_file)
                else:
                    print(f"INVALID_SUMMARY {pr.title})", file=output_file)
            if include_summary_none:
                for pr in pr_with_summary_none[curr_date]:
                    if not flatten:
                        print(f"        * [MINOR] {pr.title} (by {pr.author} in PR {pr.id})", file=output_file)
                    else:
                        print(f"MINOR {pr.title})", file=output_file)
            print(file=output_file)

        print(file=output_file)


def build_output_by_build(build_repo, pr_repo, commit_repo, output_file, include_summary_none):
    ### "ABORTED" builds have no "hash" and fucks up the logic here... but just to be sure, ignore builds without hash
    ### and changes will be atributed to next build availiable that does have a hash
    for build in build_repo.get_all_builds(filter_by=lambda x: x.last_hash is not None,
                                           sort_by=lambda x: -x.build_dttm.timestamp()):
        ### we need the previous build a hash/date hash
        ### to find commits / pull requests that got into the build
        prev_build = build_repo.get_previous_build(build.number, lambda x: x.last_hash is not None)
        if prev_build is None:
            break

        try:
            commits = list(commit_repo.get_commit_range_by_hash(build.last_hash, prev_build.last_hash))
        except MissingCommitException:
            ### we obtained half of the build's commit with our GitHub API request
            ### just avoid showing this build's partial data
            break

        print(f'BUILD {build.number} / {build.build_dttm} UTC+0 / {build.last_hash[:7]}', file=output_file, end='\n\n')
        if build.last_hash == prev_build.last_hash:
            print(f'  * No changes. Same code as BUILD {prev_build.number}.', file=output_file)
            ### I could skip to next build here, but letting the go continue could help spot bugs in the logic
            ### the code should not generate any output lines for these Builds.
            #continue

        ### I can get PRs by matching Build Commits to PRs by Merge Hash
        ### This is precise, but may fail to associate some Commits to PRs leaving few "Summaries" out
        # pull_requests = (pr_repo.get_pr_by_merge_hash(c.hash)
        #                  for c in commits if pr_repo.get_pr_by_merge_hash(c.hash))
        ### Another option is to find a proper Date range for the Build and find PRs by date
        ### But I found some issues here:
        ### * Jenkins Build Date don't end up matching the correct PRs because git fetch could be delayed like 15mins
        ### * Using Build Commit Dates is better, but a few times it incorrectly match the PR one build later
        ###   The worst ofender seem to be merges with message like "Merge remote-tracking branch 'origin/pr/25005'"
        # build_commit = commit_repo.get_commit(build.last_hash)
        # prev_build_commit = commit_repo.get_commit(prev_build.last_hash)
        # pull_requests = pr_repo.get_merged_pr_list_by_date(build_commit.commit_dttm + timedelta(seconds=2),
        #                                                    prev_build_commit.commit_dttm + timedelta(seconds=2))

        ### I'll go with the safe method, this will show some COMMIT messages instead of the proper Summary from the PR.
        pull_requests = (pr_repo.get_pr_by_merge_hash(c.hash) for c in commits if pr_repo.get_pr_by_merge_hash(c.hash))

        commits_with_no_pr = [c for c in commits if not pr_repo.get_pr_by_merge_hash(c.hash)]

        pr_with_summary = list()
        pr_with_invalid_summary = list()
        pr_with_summary_none = list()
        for pr in pull_requests:
            if pr.has_valid_summary and pr.summ_type == SummaryType.NONE:
                pr_with_summary_none.append(pr)
            elif pr.has_valid_summary:
                pr_with_summary.append(pr)
            elif not pr.has_valid_summary:
                pr_with_invalid_summary.append(pr)

        sort_by_type = lambda pr: pr.summ_type
        sorted_pr_list_by_category = sorted(pr_with_summary, key=sort_by_type)
        for pr_type, pr_list_by_category in itertools.groupby(sorted_pr_list_by_category, key=sort_by_type):
            print(f"    {pr_type}", file=output_file)
            for pr in pr_list_by_category:
                print(f"        * {pr.summ_desc} (by {pr.author} in PR {pr.id})", file=output_file)
            print(file=output_file)

        if len(commits_with_no_pr) > 0:
            print(f"    MISC. COMMITS", file=output_file)
            for commit in commits_with_no_pr:
                print(f"        * {commit.message} (by {commit.author} in Commit {commit.hash[:7]})", file=output_file)
            print(file=output_file)

        if len(pr_with_invalid_summary) > 0 or (include_summary_none and len(pr_with_summary_none) > 0):
            print(f"    MISC. PULL REQUESTS", file=output_file)
            for pr in pr_with_invalid_summary:
                print(f"        * {pr.title} (by {pr.author} in PR {pr.id})", file=output_file)
            if include_summary_none:
                for pr in pr_with_summary_none:
                    print(f"        * [MINOR] {pr.title} (by {pr.author} in PR {pr.id})", file=output_file)
            print(file=output_file)

        print(file=output_file)


if __name__ == '__main__':
    main_entry(sys.argv)
