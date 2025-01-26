# Generic Store

A generic store is a redux store template that can be used to quickly
create a redux store without the need to create slices, models, reducers
and actions.

A generic store is intended for use in situations where:

- you want access to a state from anywhere within the gui.
- you only need "set" functionality (update the state to a new value)

For those familiar with `useState`, a generic store is meant to emulate that behaviour but
with global, persistence and tab-sync properties.

Once established, generic stores can be reset to their initial value within the top right 
settings modal (where the IP addresses are set). To reset, enter the name of the generic store.


### Properties
A generic store has the following properties by default:

- can be accessed from any component
- persists after tab refresh
- syncs across tabs

The later two can be disabled if needed when creating a generic store.

## useGenericStore

```typescript
const [value, setValue] = useGenericStore<type>("storeName");
```

### Returns
useGenericStore returns an array with exactly two values:

- The current state.
- The set function that lets you update the state to a different value.

## Setup

To set up a generic store the following steps must be followed:

### 1. Define store and type in `redux/RootState.ts`

```typescript
export interface RootState {
  // ...
  
  // Generic stores
  // example:
  currentSite: GenericStoreState<Site>;
  
  // add yours here in the form:
  <store name>: GenericStoreState<type of store>
}
```

if additional type definitions are needed (eg a custom interface) please 
define the interface/type definition in the `redux/models/genericStores` folder.

Please note that union types or interfaces with optional fields won't work due to internal
validation that occurs.

### 2. Define store and initial value in `redux/RootReducer.ts`

Ensure that store name matches here

```typescript
export const reduxStores = {
  // ...
  
  // Generic stores
  // example:
  currentSite: createGenericStore("currentSite", SITE.SITE_1),
  
  // add yours here in the form:
  <store name>: createGenericStore("<store name>", <initial value>),
}
```

#### CreateGenericStore

`CreateGenericStore` takes in 2-4 arguments:

- `name`: the name of the store.
- `initialValue`: the initial value of the store.
- `shouldPersist`: optional, whether the store should persist, defaults to true
- `shouldTabSync`: optional, whether the store should tab sync, defaults to true

if you wish for a store to not persist or tab sync the respective field must be set
to false.

### 3. Use hook 

Here value can be any name, but `store name` must be the same as what
was defined before.

```typescript
const [<value>, <setValue>] = useGenericStore<type>("<store name>")
```

And that's it, you're ready to go!