# Generic Store

A generic store is a redux store template that can be used to quickly
create a redux store without the need to create slices, models, reducers
and actions.

The generic store is only intended for use in situations like useState, when
you want access to the current value and to set the new value.

example use case:

```typescript
const [currentSite, setCurrentSite] = useGenericStore<number>("currentSite");
```
`currentSite` and `setCurrentSite` can then be used exactly like `useState`.

`useGenericStore` has the following properties by default:

- can be accessed from any component
- persists after tab refresh 
- syncs across tabs


## Setup

To set up a generic store the following steps must be followed:

### 1. Define store and type in `redux/RootState.ts`

```typescript
export interface RootState {
  // ...
  
  // Generic stores
  // example:
  currentSite: GenericStoreState<number>;
  
  // add yours here in the form:
  <store name>: GenericStoreState<type of store>
}
```

if additional type definitions needed (eg need a custom interface) please 
define the interface/type definition in the `redux/models/genericStores` folder.

### 2. Define store and initial value in `redux/RootReducer.ts`

Ensure that store name matches here

```typescript
export const rootReducer = combineReducers({
  // ...
  
  // Generic stores
  // example:
  currentSite: createGenericStore("currentSite", 0),
  
  // add yours here in the form:
  <store name>: createGenericStore("<store name>", <initial value>),
})
```

### 3. Use hook 

Here value can be any name, but `store name` must be the same as what
was defined before.

```typescript
const [<value>, <setValue>] = useGenericStore("<store name>")
```

## Persist and Tab syncing

By default, any generic stores will persist and sync across tabs, this can
be disabled if required.

### Disable Tab Sync

To disable tab sync add the store name to the tab sync middleware blacklist
in `redux/store/configureRootStore.ts` 

```typescript
  const tabSyncConfig = {
  // blacklist of actions to be synced across tabs upon update
  // any actions that should not be synced across tabs should be added here
  predicate: tabSyncPredicate({
    stores: [
      "cameraStreamerState",
      "persist",
      
      // add store name here
      "<store name>",
      
    ],
    actions: [
      UIActions.SETTINGS_MODAL_UPDATE.toString(),
      UIActions.CONTROLLER_HELP_MODAL_UPDATE.toString(),
      UIActions.SIDEBAR_UPDATE.toString(),
      UIActions.BLCMD_STATUS_MODAL_UPDATE.toString(),
    ],
  }),
  // the function that is called for every action that matches one of the whitelisted actions.
  effect: tabSyncMiddleware(),
};
```

### Disable redux-persist

To disable state persistence add the store name to the redux persist 
blacklist config in `redux/store/configureRootStore.ts`

```typescript
const persistConfig = {
  key: 'root',
  storage,
  // stores that should not be persisted includes all bifrost stores
  // any stores that should not be persisted should be added here.
  blacklist: [
    ...bifrostStores,
    "cartographerState",
    "cameraStreamerState",
    
    // add store name here
    "<store name>",
  ]
};
```

