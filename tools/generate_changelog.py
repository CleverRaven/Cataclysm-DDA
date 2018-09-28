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
from datetime import date, datetime, timedelta


log = logging.getLogger('generate_changelog')


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

    SUMMARY_REGEX = r'SUMMARY:\s+(?P<pr_type>\w+)\s*(?:"(?P<pr_desc>.+)")?'

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

    def get_commit(self, hash_id):
        return self.ref_by_commit_hash[hash_id] if hash_id in self.ref_by_commit_hash else None

    def get_latest_commit(self):
        return self._latest_commit

    def get_all_commits(self):
        for commit in self.ref_by_commit_hash.values():
            yield commit

    def traverse_commits_by_first_parent(self, initial_commit=None):
        """Iterate through Commits connected by the first Parent, until a parent is not found in the Repository

        This is like using 'git log --first-parent $commit', which avoids Commits "inside" Pull Requests / Merges.
        But returns Merge Commits (which often can be related to a single PR) and Commits directly added to a Branch.
        """
        if initial_commit is not None:
            commit = initial_commit
        else:
            commit = self._latest_commit

        while commit is not None:
            yield commit
            commit = self.get_commit(commit.parents[0])

    def purge_references(self):
        self.ref_by_commit_hash.clear()


class CDDAPullRequestRepository:
    """Groups Pull Requests for storage and common operations"""

    def __init__(self):
        self.ref_by_merge_hash = {}

    def add(self, pull_request):
        if pull_request.merge_hash is not None:
            self.ref_by_merge_hash[pull_request.merge_hash] = pull_request

    def add_multiple(self, pull_requests):
        for pull_request in pull_requests:
            self.add(pull_request)

    def get_pr_by_merge_hash(self, merge_hash):
        return self.ref_by_merge_hash[merge_hash] if merge_hash in self.ref_by_merge_hash else None

    def get_all_pr(self):
        for pr in self.ref_by_merge_hash.values():
            yield pr

    def purge_references(self):
        self.ref_by_merge_hash.clear()


class CommitApi:

    def __init__(self, commit_repo, api_token):
        self.commit_repo = commit_repo
        self.api_token = api_token

    def get_commit_list(self, min_commit_dttm, branch='master', max_threads=15):
        """Return a list of Commits from specified commit date up to now. Order is not guaranteed by threads.

            params:
                min_commit_dttm = None or minimum commit date to be part of the result set (UTC+0 timezone)
        """
        results_queue = collections.deque()
        request_generator = CommitApiGenerator(self.api_token, min_commit_dttm, branch)
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

        return self.commit_repo.create(commit_sha, commit_message, commit_dttm, commit_author, commit_parents)


class PullRequestApi:

    GITHUB_API_SEARCH = r'https://api.github.com/search/issues'

    def __init__(self, pr_repo, api_token):
        self.pr_repo = pr_repo
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

    def get_pr_list(self, min_dttm, state='all', merged_only=False, max_threads=15):
        """Return a list of PullRequests from specified commit date up to now. Order is not guaranteed by threads.

            params:
                min_dttm = None or minimum update date on the PR to be part of the result set (UTC+0 timezone)
                state = 'open' | 'closed' | 'all'
                merged_only = search only 'closed' state PRs, and filter PRs by merge date instead of update date
        """
        results_queue = collections.deque()
        request_generator = PullRequestApiGenerator(self.api_token, state if not merged_only else 'closed')
        threaded = MultiThreadedGitHubApi()
        threaded.process_api_requests(request_generator,
                                      self._process_pr_data_callback(results_queue, min_dttm, merged_only),
                                      max_threads=max_threads)

        if merged_only:
            return (pr for pr in results_queue if pr.is_merged and pr.merged_after(min_dttm))
        else:
            return (pr for pr in results_queue if pr.updated_after(min_dttm))

    def _process_pr_data_callback(self, results_queue, min_dttm, merged_only):
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

        return self.pr_repo.create(pr_number, pr_title, pr_author, pr_state, pr_body,
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
            t = threading.Thread(target=self._exit_on_exception(self._process_api_requests_on_threads),
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

    @staticmethod
    def _exit_on_exception(func):
        """Decorator to terminate the main script and all threads if a thread generates an Exception"""
        def _exit_on_exception_closure(*args, **kwargs):
            try:
                func(*args, **kwargs)
            except Exception as err:
                log.exception(f'Unhandled Exception: {err}')
                os._exit(-5)
        return _exit_on_exception_closure


class GitHubApiRequestBuilder:

    def __init__(self, api_token):
        self.api_token = api_token

    def create_request(self, url, params=None):
        """Creates an API request based on provided URL and GET Parameters"""
        if params is None:
            request_url = url
        else:
            request_url = url + '?' + urllib.parse.urlencode(params)

        if self.api_token is None:
            api_request = urllib.request.Request(request_url)
        else:
            auth_headers = {'Authorization': 'token ' + self.api_token}
            api_request = urllib.request.Request(request_url, headers=auth_headers)

        return api_request


class CommitApiGenerator(GitHubApiRequestBuilder):
    """Generates multiple HTTP requests to get Commits, used from Threads to get data until a condition is met."""

    GITHUB_API_LIST_COMMITS = r'https://api.github.com/repos/CleverRaven/Cataclysm-DDA/commits'

    def __init__(self, api_token, since_dttm=None, sha='master', initial_page=1, step=1):
        super().__init__(api_token)
        self.sha = sha
        self.since_dttm = since_dttm
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
                req = self.create_request(self.since_dttm, self.sha, self.page)
                self.page += self.step
                return req
            else:
                return None

    def create_request(self, since_dttm=None, sha='master', page=1):
        """Creates an HTTP Request to GitHub API to get Commits from CDDA repository."""
        params = {
            'sha': sha,
            'page': page,
        }
        if since_dttm is not None:
            params['since'] = since_dttm.isoformat()

        return super().create_request(self.GITHUB_API_LIST_COMMITS, params)


class PullRequestApiGenerator(GitHubApiRequestBuilder):
    """Generates multiple HTTP requests to get Pull Requests, used from Threads to get data until a condition is met."""

    GITHUB_API_LIST_PR = r'https://api.github.com/repos/CleverRaven/Cataclysm-DDA/pulls'

    def __init__(self, api_token, state='all', initial_page=1, step=1):
        super().__init__(api_token)
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


def do_github_request(api_request, retry_on_limit=3):
    """Do an HTTP request to GitHub and retries in case of hitting API limits"""
    for retry in range(1, retry_on_limit + 2):
        try:
            with urllib.request.urlopen(api_request) as api_response:
                log.debug(f'Request DONE {api_request.full_url}')
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


def main_entry(argv):
    parser = argparse.ArgumentParser(description=
        'Generates Changelog from now until the specified date'
    )

    parser.add_argument(
        'target_date',
        help='Specify when should stop generating . Accepts "YYYY-MM-DD" ISO 8601 Date format.',
        type=lambda d: datetime.combine(date.fromisoformat(d), datetime.min.time())
    )

    parser.add_argument(
        '-t', '--token-file',
        help='Specify where to read the Personal Token. Default "~/.generate_changelog.token".',
        default='~/.generate_changelog.token'
    )

    parser.add_argument(
        '-o', '--output-file',
        help='Specify where to write extracted information. Default to standard output.',
        default=None
    )

    parser.add_argument(
        '--verbose',
        action='store_true',
        help='Indicates the logging system to generate more information about actions.',
        default=None
    )

    arguments = parser.parse_args(argv[1:])

    logging.basicConfig(level=logging.DEBUG if arguments.verbose else logging.INFO,
                        format='   LOG | %(threadName)s | %(levelname)s | %(message)s')
    log.debug(f'Commandline Arguments (+defaults): {arguments}')
    main(arguments.target_date, arguments.token_file, arguments.output_file)


def main(target_dttm, token_file, output_file):
    personal_token = read_personal_token(token_file)

    if output_file is None:
        output_file = sys.stdout
    else:
        output_file = open(pathlib.Path(str(output_file)).expanduser(), 'w', encoding='utf8')

    commit_api = CommitApi(CommitFactory(), personal_token)
    commit_repo = CommitRepository()
    commit_repo.add_multiple(commit_api.get_commit_list(target_dttm))

    pr_api = PullRequestApi(CDDAPullRequestFactory(), personal_token)
    pr_repo = CDDAPullRequestRepository()
    pr_repo.add_multiple(pr_api.get_pr_list(target_dttm, merged_only=True))

    ### group commits with no PR by date
    commits_with_no_pr = collections.defaultdict(list)
    for commit in commit_repo.traverse_commits_by_first_parent():
        if not pr_repo.get_pr_by_merge_hash(commit.hash):
            commits_with_no_pr[commit.commit_date].append(commit)

    ### group PRs with summary by date
    pr_with_summary = collections.defaultdict(list)
    for pr in pr_repo.get_all_pr():
        if pr.has_valid_summary and pr.summ_type != SummaryType.NONE:
            pr_with_summary[pr.merge_date].append(pr)

    ### build main changelog output
    for curr_date in (datetime.now().date() - timedelta(days=x) for x in itertools.count()):
        if curr_date < target_dttm.date():
            break
        if curr_date not in pr_with_summary and curr_date not in commits_with_no_pr:
            continue

        print(f"{curr_date}", file=output_file)
        sort_by_type = lambda pr: pr.summ_type
        pr_list_by_date = sorted(pr_with_summary[curr_date], key=sort_by_type)
        for pr_type, pr_list_by_category in itertools.groupby(pr_list_by_date, key=sort_by_type):
            print(f"    {pr_type}", file=output_file)
            for pr in pr_list_by_category:
                print(f"        * {pr.summ_desc} (by {pr.author} in PR {pr.id})", file=output_file)
            print(file=output_file)

        if curr_date in commits_with_no_pr:
            print(f"    MISC COMMITS", file=output_file)
            for commit in commits_with_no_pr[curr_date]:
                print(f"        * {commit.message} (by {pr.author} in Commit {commit.hash})", file=output_file)
        print(file=output_file)

    ### output PRs with no Summary
    sort_by_author = lambda pr: pr.author
    for pr in sorted(filter(lambda pr: not pr.has_valid_summary, pr_repo.get_all_pr()), key=sort_by_author):
        print(f"No Summary for PR {pr.id} / DATE {pr.merge_date} / AUTHOR {pr.author} / TITLE {pr.title}",
              file=output_file)

    ### some empty lines to separate sections
    print(os.linesep * 2, file=output_file)

    ### probably not that interesting, but output PRs with Summary = 'None'
    ### which means Dev decided to exclude his PR from Changelog)
    for pr in sorted(filter(lambda x: x.summ_type == SummaryType.NONE, pr_repo.get_all_pr()), key=sort_by_author):
        print(f"Summary = 'None' for PR {pr.id} / DATE {pr.merge_date} / AUTHOR {pr.author} / TITLE {pr.title}",
              file=output_file)


if __name__ == '__main__':
    main_entry(sys.argv)
