// ./tools/scripts/generate-release-notes.js

const github = require('@actions/github');

/**
 * Generates the release notes for a github release.
 *
 * Arguments:
 * 1 - github_token
 * 2 - new version
 * 3 - commit SHA of new release
 */
const token = process.argv[2];
const version = process.argv[3];
const comittish = process.argv[4];
const repo = process.env.REPOSITORY_NAME
const owner = process.env.GITHUB_REPOSITORY_OWNER

function format_request_error(error) {
    // Octokit promises that all errors are https://github.com/octokit/request-error.js
    try {
        let out = `${error} (error ${error.name} code ${error.status})`;
        if (error.response?.data){
            // the response data is not the raw bytes we get from the server, but already
            // preprocessed and typified object. There's *probably* not much extra info
            // we can glean from this, but we shall try regardless
            out += "\n";
            let data = error.response.data;
            for(let key of Object.keys(data)){
                out += `  ${key}: ${data[key]}\n`
            }
        }
        return out;
    } catch (e) {
        return `${error}`;
    }
}

async function main() {
  const client = github.getOctokit(token);

  const latestReleaseResponse = await client.request(
    'GET /repos/{owner}/{repo}/releases',
    {
      owner: owner,
      repo: repo,
      headers: {
        'X-GitHub-Api-Version': '2022-11-28',
      },
    }
  ).catch((e) =>{
    throw `${format_request_error(e)} ...when getting latest release`;
  })

  if (latestReleaseResponse.data) {
    for (const responseData of latestReleaseResponse.data) {
      if (responseData.draft == false && responseData.prerelease == true) {
        previousTag = responseData.tag_name;
	break;
      }
    }
  }

  const response = await client.request(
    'POST /repos/{owner}/{repo}/releases/generate-notes',
    {
      owner: owner,
      repo: repo,
      tag_name: version,
      previous_tag_name: previousTag,
      target_commitish: comittish,
      headers: {
        'X-GitHub-Api-Version': '2022-11-28',
      },
    }
  ).catch((e) =>{
    throw `${format_request_error(e)} ...when asking github to autogenerate release notes since tag '${previousTag}'`;
  });

  const noteSections = response.data.body?.split('\n\n');
  const trimmedSections = [];
  const githubNotesMaxCharLength = 125000;
  const maxSectionLength = githubNotesMaxCharLength / noteSections.length;
  for (let i = 0; i < noteSections.length; i++) {
    if (noteSections[i].length > githubNotesMaxCharLength) {
      const lastLineIndex =
        noteSections[i].substring(0, maxSectionLength).split('\n').length - 1;
      const trimmed =
        noteSections[i]
          .split('\n')
          .slice(0, lastLineIndex - 1)
          .join('\n') +
        `\n... (+${
          noteSections[i].split('\n').length - (lastLineIndex + 1)
        } others)`;
      trimmedSections.push(trimmed);
      continue;
    }

    trimmedSections.push(noteSections[i]);
  }

  console.log(trimmedSections.join('\n\n'));
}

main().catch((e) => {
  console.error(`Failed generating release notes with error: ${e}`);
  process.exit(0);
});
