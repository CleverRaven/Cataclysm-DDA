import requests
from datetime import datetime
import urllib
from concurrent.futures.thread import ThreadPoolExecutor 
from concurrent.futures import as_completed
import os
import sys 

# If you don't have a github api key, you are likely to be rate limited.
# get one here https://github.com/settings/tokens
# should look like this ghp_cCx5F6xTSn07hbSRxZW2pbFsNFyiQPCx5K19

API_KEY =  os.environ["key"]
REPO_API = 'https://api.github.com/repos/CleverRaven/Cataclysm-DDA/'

def github_fetch(path,parms={}):
    header = {'Authorization': f'token {API_KEY}'}
    querystring = urllib.parse.urlencode(parms)
    r = requests.get(f"{REPO_API}{path}?{querystring}",headers=header)
    return r.json()

def get_releases(page=1):
    return github_fetch('releases',{'page':page,'per_page':100})

def match_commit_to_pr(hash):
    json = github_fetch(f'commits/{hash}/pulls')
    if len(json) == 0: # likely being rate limited
        sys.stderr.write("You might be being rate limited")
        raise Exception()
    return json[0]

def get_cat(body):
    # I tried using regular expression, but it kept throwing exceptions.
    VALID_SUMMARY_CATEGORIES = (
        'content',
        'features',
        'interface',
        'mods',
        'balance',
        'i18n',
        'bugfixes',
        'performance',
        'build',
        'infrastructure',
        'none',
    )
    body = body.lower().replace('\r\n',' ').replace("\n"," ").replace(':','')
    s = [x for x in body.split(' ') if x != '']
    for a,b in zip(s,s[1:]):
        if a == 'summary':
            if b in VALID_SUMMARY_CATEGORIES:
                return b
    return 'unable to determine category'

def get_commits_in_time_range(starting_date,ending_date):
    current_timestamp = ending_date
    page_num = 1

    commits_hashes = []

    while current_timestamp > starting_date:
        releases = get_releases(page_num)
        for release in releases:
            commits_hashes.append(release['target_commitish'])
            current_timestamp = datetime.fromisoformat(release['published_at'][:-1])
            if current_timestamp < starting_date:
                break
        page_num+=1
    
    return commits_hashes


def generate_changelogs(starting_date,ending_date=None):
    if ending_date == None:
        ending_date = datetime.today()

    commits_hashes = get_commits_in_time_range(starting_date,ending_date)
    d = dict()

    with ThreadPoolExecutor() as executor: # multi threading
        res = [executor.submit(match_commit_to_pr, hash) for hash in commits_hashes]
        for future in as_completed(res):
            info = future.result()
            author = info['user']
            txt_line = f"[{info['title']}]({info['html_url']}) by [{author['login']}]({author['html_url']})"
            cat = get_cat(info['body'])

            if cat not in d:
                d[cat] = []
            d[cat].append(txt_line)

    sys.stdout.write(f'Logs generated from {starting_date} til {ending_date}\n')
    for k in d:
        sys.stdout.write(f'# {k[0].upper()}{k[1:]}\n')
        for i in d[k]:
            sys.stdout.write(i+'\n\n')


if __name__ =='__main__':
    starting_time = '2021-10-20'
    generate_changelogs(datetime.fromisoformat(starting_time))
