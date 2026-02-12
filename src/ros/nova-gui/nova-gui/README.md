# Nova-GUI Frontend

This directory is the root directory of the frontend that constitutes Nova-GUI. Basic terminology and Best practices are outlined below.

## Folder Structure

<!-- Taken from PR #1 -->

The Folder Structure's been Organised into the following

- **assets**: Store all non typescript stuff such as Images, Fonts and other misc stuff
- **routes**: Store all routing related stuff
- **ros**: Store ROS Related stuff. The ROS TS Generator would output to this folder.
- **redux**: All Redux and Bifrost Related Stuff
- **views**: Views in this case refers to the different "Profiles". Each view will contain It's Own Layouts and other components which are specific to the purpose.
- **components**: This is our effective component library containing all components used.

## Component Development Lifecycle

Designing and Drafting a Component can include a myriad of Confusion and Ambiguity. This section aims at making this a bit more easy for people to onboard and design their components.

All components on the Components Folder needs to be Generic and Monolithic in Nature, i,e one should be able to use this components across views without any difficulty. All Hooks, functions, enums and types should be stored in the component folder. It's recommended to substructure a component if needed.The shared folder houses components that is used in other components.

A component can do one or more of the following functions

- Stream data from the Rover
- Recieve on demand data from the Rover (like a REST API)
- Send a command to the Rover to perform a Task
- Please the Rover Operator by looking Cute and Cheerfull

## Development

We use the [`yarn` package manager](https://yarnpkg.com/cli). To use yarn you must be in a nix-shell with yarn, this can be achieved by running `gui-shell`. To run `yarn` commands you can run one of:

```bash
# you must be in the yarn project directory: /nova/src/ros/nova-gui/nova-gui
yarn

# can be run anywhere
gui-yarn
# alias for:
yarn --cwd ~/nova/src/ros/nova-gui/nova-gui
```

External packages are managed through `yarn` and defined in the package.json file. You can search external dependancies [here](https://www.npmjs.com/). To install new dependencies, run:

```bash
yarn add <package-name>
```

All external packages are installed into the `node_modules` directory and can be imported directly into the project’s source files as needed.

To install the project's dependancies run:

```bash
yarn install
```

The GUI can be built, if it fails to build `ws-build` will fail, impacting all development. Please ensure that you run the follow before raising a PR/merging:

```bash
yarn build
```

### Linting

`eslint` has been set up for this repository, to utilise this locally, you can run in the development shell:

```shell
yarn lint
```

This will display any and all linting issues, please try and fix any issues that arise. If you are unsure what the issue is/don't know how to fix it, don't hesitate to reach out to the team.

If you think your use-case is a valid case to ignore the linting warnings please place this line in front of the offending line:

```typescript
// template:
// eslint-disable-next-line <rule to ignore>

// example:
// eslint-disable-next-line react-hooks/exhaustive-deps
```

Please try to ensure that there are no linting issues when merging in, there is a github workflow (eslint) that will pick up and give warnings for any linting errors/warnings.
