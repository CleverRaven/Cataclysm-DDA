import * as core from '@actions/core';
import * as github from '@actions/github';

async function run() {
  try {
    const description-regex = core.getInput('description-regex');
    core.debug(`Applying ${description-regex} to description string.`);
    const description = github.context!.payload!.pull_request!.body;
    console.log('PR Description: $(description}');
    
    if (!description.match(new RegExp(description-regex))) {
        core.setFailed('Please fix your PR SUMMARY line to match ${description-regex}');
    } else {
        console.log('PR description contains valid SUMMARY line.');
    }
  } catch (error) {
    core.setFailed(error.message);
  }
}

run();
