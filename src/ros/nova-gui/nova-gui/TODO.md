# HeroUI v3 / Tailwind v4 Migration

## Completed in this session

- Replaced all unresolved legacy HeroUI/NextUI exports. `yarn tsc --noEmit` no
    longer reports `has no exported member` errors.
- Migrated `CardBody` to `CardContent` across the source tree.
- Replaced direct v3-equivalent component names: `ModalContent` ->
    `ModalDialog`, `Textarea` -> `TextArea`, `Progress` -> `ProgressBar`, and
    `Divider` -> `Separator`.
- Removed the remaining direct `@nextui-org/react` import.
- Replaced `useDisclosure` with `useOverlayState`, preserving existing local
    `onOpen` and `onOpenChange` names through destructuring aliases.
- Reworked root navigation from removed `Navbar` components to semantic layout
    elements and v3 `Separator`.
- Migrated root settings tabs, YOLO selector, top-bar dropdowns, and radio-modal
    tabs/dropdown to v3 collection composition.
- Updated shared `OverlayedProgress`, `CopyableInput`, `CopyableOutput`, and
    `SpinnerButton` wrappers to stop forwarding removed v2 contracts.
- Converted visible v2 select/tab structures in the site selector, segmented
    picker, camera controls, NIR controls, Cartographer controls, and several
    science widgets to v3 trigger/popover/listbox or tab-list composition.
- Deferred closed legacy modal/sidebar content so it does not prevent unrelated
    routes from mounting. Each deferred dialog still needs full v3 composition
    before it is opened.
- Verified the root GUI renders at `http://127.0.0.1:5174/` with no page error.
    Confirmed camera paths and several general, base, test, and science paths
    render during the migration sweep.

## Current Baseline

Run commands from this directory with Yarn:

```sh
yarn tsc --noEmit
yarn lint
yarn dev
```

- `yarn tsc --noEmit`: 427 errors as of this handoff, reduced from 693. There
    are no remaining unresolved HeroUI exports.
- `yarn lint`: has pre-existing/non-migration findings; do not use it as the
    migration gate until those are handled separately.
- The development server has been exercised with the browser. Re-run only paths
    that were previously failing after each focused component migration.

## Handoff Context For The Next Agent

### Workspace and Constraints

- Work in `/home/nova/nova/src/ros/nova-gui/nova-gui`.
- Use Yarn for project commands. Do not use npm or pnpm.
- This is a deliberately broad, uncommitted migration: `git diff` currently
    contains 139 modified files, about 806 additions and 875 deletions. Do not
    revert unrelated changes or attempt to restart the migration from scratch.
- The app is a Vite/React application. Route definitions are in
    `src/routes/routes.tsx`; the shared shell is `src/RosRoot.tsx`.
- Follow this loop: run `yarn tsc --noEmit`, select one component owner from the
    output, make one focused migration, rerun TypeScript for that owner or the
    full check, then use the existing `yarn dev` server only to test the route or
    interaction affected by that component.
- Do not retest already-confirmed working paths after every patch. Test the
    previously failing route or newly opened dialog that the current patch owns.

### Verified HeroUI v3 API Lessons

These were confirmed against the installed `@heroui/react` v3.2.4 declarations:

- `CardBody` was renamed to `CardContent`; `CardHeader` and `CardFooter` remain
    exports.
- `ModalContent` was renamed to `ModalDialog`. A complete v3 modal needs the
    root/state model and appropriate backdrop/container/dialog composition;
    simply renaming the component removes the import error but does not complete
    the behavioral migration.
- `Textarea` is `TextArea`; `Progress` is `ProgressBar`; `Divider` is
    `Separator`.
- `useDisclosure` does not exist. Use `useOverlayState()` and alias methods at
    existing call sites when needed:

    ```tsx
    const { isOpen, open: onOpen, setOpen: onOpenChange } = useOverlayState();
    ```

- `Select` requires `Select.Trigger`, `Select.Value`, `Select.Indicator`,
    `Select.Popover`, and a `ListBox` containing `ListBoxItem` entries. A bare
    `ListBoxItem` causes `cannot be rendered outside a collection` at runtime.
- `Tabs` requires `TabList` containing `Tab id="..."` children, plus matching
    `TabPanel id="..."` children. V2 `Tab title="..."` is invalid.
- `Dropdown` requires `DropdownTrigger`, then `Dropdown.Popover`, then
    `DropdownMenu` and `DropdownItem id="..."`. Do not place a menu directly
    under the dropdown root.
- `Tooltip` requires trigger/content composition. V2 `content` and root
    styling props are not supported on the v3 root.
- Button visual state is now primarily `variant` plus classes. Do not forward
    v2 `color`, `radius`, `shadow`, `startContent`, `endContent`, or `isLoading`
    to `Button` without checking the v3 type declaration.
- V3 `Input` is a native-input wrapper, not v2's composite field. It does not
    accept v2 `label`, `description`, `startContent`, `endContent`, or
    `onValueChange`; use adjacent label/description elements and `onChange`.
- `ProgressBar` still accepts `color`, but needs explicit
    `ProgressBar.Track` and `ProgressBar.Fill` children.

### Temporary Render Guards To Revisit

These guards were intentional migration containment, not permanent feature
changes. They prevent closed components with unfinished v2 compositions from
breaking unrelated routes. Remove each only after migrating and testing the
opened surface:

- `src/RosRoot.tsx`: settings, controller-help, and BLCMD status modals.
- `src/components/navbar/RadioStatusModal/RadioStatusButton.tsx`: radio modal.
- `src/components/cameras/CameraPage/CameraControlModelButton.tsx` and
    `src/components/cameras/CameraPage/CamerasPage.tsx`: camera control panel.
- `src/views/shared/CamerasPage/CamerasView.tsx`: camera sidebar.
- `src/components/maps/Cartographer/Cartographer.tsx`: new-marker modal.
- `src/views/urc/URCGazebo.tsx`: camera control panel.

`BLCMDStatusButton` no longer mounts its own duplicate modal; the root owns the
single guarded BLCMD modal instance.

### Runtime Status Learned So Far

- The root GUI shell has rendered successfully at `http://127.0.0.1:5174/`.
- Camera paths became renderable after guarding closed camera modals/sidebar.
- A previous browser sweep found failures concentrated in NIR, Cartographer,
    science, and autonomous-navigation surfaces. Several visible select/tab/menu
    controls in those areas were migrated afterwards; rerun only those previously
    failing paths to establish the current route baseline.
- A route rendering with closed guards does not mean its dialogs, popovers,
    tables, or settings forms are migrated. Open and exercise those interactions
    before calling the migration complete.

## Remaining Work

The HeroUI full-migration guide's main requirement still applies: v3 components
use React Aria composition and slot-based styling rather than v2's monolithic
props. The supplied guide URL returned 404 to the documentation fetcher during
this session, so verify current examples in the guide before making each
component-family conversion.

### 1. Convert Remaining v2 Prop Contracts

There are 344 `TS2322` errors. Prioritize shared wrappers and highest-error
owners first:

- `color`: 68 compiler errors. For `Button`, use v3 `variant` plus Tailwind
    classes when semantic status colors are needed. Keep `color` only on v3
    components that explicitly support it, such as `ProgressBar`.
- `label`, `description`, `startContent`, and `endContent`: move content into
    v3 child composition or render adjacent semantic elements. Native v3 `Input`
    does not accept v2 field slot props.
- `onValueChange`: use `onChange` for native inputs, `onSelectionChange` for
    select/listbox controls, and `onChangeEnd` for sliders as dictated by the v3
    component type.
- `size`, `radius`, `shadow`, `classNames`, `removeWrapper`, `isCompact`,
    `isHeaderSticky`, `emptyContent`, `align`, `placement`, `isLoading`, and
    `disableAnimation`: replace with documented v3 props or Tailwind classes.

Highest-concentration files from the current check:

- `src/components/science/Potentiostat/CalibrationMenu.tsx`
- `src/components/science/RamanSpec/RamanMechanicalInputs.tsx`
- `src/components/shared/widgets/GenericGraphComparisonWidget/GenericGraphComparisonWidget.tsx`
- `src/components/science/ToolRotatorWidget/ToolRotatorWidget.tsx`
- `src/components/science/NIRProbe/ARCNIRProbeWidget.tsx`
- `src/components/maps/Cartographer/components/NewMarkerModal.tsx`
- `src/views/urc/URCScienceView.tsx`
- `src/components/science/UVVisSpec/UVVisSpec.tsx`

### 2. Finish Modal and Popover Composition

V3 modal roots require `useOverlayState` via `state` or the documented v3
controlled pattern, then a `Modal.Backdrop`, `Modal.Container`, and
`Modal.Dialog` hierarchy. Existing `isOpen`, `onClose`, root `className`, and
size props are v2 contracts and must move to the appropriate v3 layer.

Apply the same principle to popovers and dropdowns: trigger, popover/content,
then menu/listbox. All `DropdownItem` values must be inside `DropdownMenu` and
use v3 IDs/keys according to its collection API.

### 3. Finish Tables, Selects, Tabs, and Sliders

- Tables: retain `TableHeader`/`TableBody`/`TableRow`/`TableCell` collections,
    but replace v2 table wrapper, sticky-header, empty-state, and alignment props
    with v3 structure and classes.
- Selects: use `Select.Trigger`, `Select.Value`, `Select.Indicator`,
    `Select.Popover`, and `ListBox`/`ListBoxItem`. Do not render `ListBoxItem`
    directly under `Select`.
- Tabs: use `Tabs`, `TabList`, `Tab`, and `TabPanel`; do not use v2 `title`
    props or direct `Tab` children under `Tabs`.
- Sliders: add the v3 track/fill/thumb child tree and migrate v2 labels,
    thumb icons, and value callbacks.

### 4. Restore Deferred Dialogs and Verify All Paths

Once a modal/sidebar has been migrated, remove its temporary conditional mount
guard and test open, close, keyboard dismissal, and action callbacks. Then run
`yarn tsc --noEmit` and visit every route listed in `src/routes/routes.tsx`,
including camera variants and opened dialogs/popovers.

### 5. Tailwind v4 Follow-up

The HeroUI v3/Tailwind v4 setup currently loads the HeroUI plugin and source
glob. Verify semantic utility classes (`text-default-*`, `bg-default-*`,
`text-h1`, `rounded-small`, etc.) in the running UI. Replace any v2-only token
that no longer emits CSS with standard Tailwind v4 utilities or documented
HeroUI v3 theme tokens.

## Suggested Commit Message

```text
refactor(gui): begin HeroUI v3 and Tailwind v4 component migration

Replace removed NextUI/HeroUI v2 exports, migrate core collection composition,
and update shared UI wrappers so the root GUI and camera routes can render on
HeroUI v3. Leave remaining v2 prop-contract conversions tracked in TODO.
```