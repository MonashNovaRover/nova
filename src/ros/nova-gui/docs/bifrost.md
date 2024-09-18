## Bifrost

![Bifrost](./assets/Bifrost.webp)

> Behold! The Burning Rainbow bridge that gods themselves traverse, transcending the realms of Asgard and venturing into the vast expanse of the 8 realms, including the sacred Midgard, known to mortals as Earth. This ethereal passage, this celestial conduit, is none other than [Bifrost](https://en.wikipedia.org/wiki/Bifr%C3%B6st), a testament to the divine power that binds the cosmos.

In the context of Nova-GUI, Bifrost is the implementation of `roslibjs` and `redux-toolkit` combined to provide seamless access to ROS Topics and Services directly from React Components without worrying about communication, types and other nitty gritty things. `roslibjs` is used to connect to the `rosbridge_server` through means of websockets which enables us to recieve real time updates from ROS.

### How to Summon Bifrost

Alas! We do not possess the mighty Mjolnir or the watchful eye of Heimdall to summon the majestic Bifrost. Oh, how simple life would be if we could invoke Thor's hammer within GUI. But fear not, for there exists a power even greater than that of Mjolnir itself - [React Hooks](https://react.dev/reference/react/hooks)!! With the boundless might of React Hooks, we have crafted a solution that is not only robust and developer-friendly, but also wields the power to seamlessly connect ROS to the very fabric of the GUI using the custom hook `useBifrost`

### `useBifrost`

Sorry you had to go thru that description before you're reading this (Kabi is Sorry). Essentially `useBifrost` allows you to subscribe to topics and send service requests through Bifrost. The hook returns the `bifrost` object for use inside component. To use the hook, use the following inside a component.

```typescript
const bifrost = useBifrost({topic: RosTopic.<YOUR_TOPIC_HERE>, service: RosService.<YOUR_SERVICE_HERE>})
```

A Simplified form might be

```typescript
const bifrost = useBifrost(BifrostProps);
```

where `BifrostProps` is an Object containing A RosTopic or a RosService ( or Both ) .

> [!IMPORTANT] `useBifrost` needs atleast a topic or a service to work, adding both will work just as fine and is recommended if the component uses both topics and services

To use a Topic or a Service, the entity must be added to the `ros` directory and some setup steps are required. These steps are documented under [Bifrost Services](./bifrost_services.md) for Services and [BifrostTopics](./bifrost_topics.md) for Topics.

The `bifrost` Object has some powerful methods that are available for use in components.

#### `bifrost.syncWithTopic()`

This method uses the `BifrostProps.topic` passed to `useBifrost` to open up a subscription for the topic specified. The Changes are always passed on to Redux, and can be easily access using the `useSelector` hook and tapping into the `RootState`.

> [!IMPORTANT] since we only need to place the subscription once. It is absolutely necessary to wrap this function with a `useEffect` at all times. This function can however be used as callback functions and so on.

```ts
useEffect(() => {
  bifrost.syncWithTopic();
}, []); // Do not Add Bifrost to the dependency array
```

#### `bifrost.callService()`

This method uses the `Bifrost.service` passed to `useBifrost` to call the specifed ROS Service. Some parameters / inputs are needed by `callService` to call the function properly. The Format of the function looks like this

```ts
bifrost.callService(Request, CallServiceOption);
```

**Request:** The Request that is to be sent to ROS. This is a regular Object whose type is configured in [`rosServiceTypes.ts`](../nova-gui/src/ros/services/rosServiceTypes.ts).

**CallServiceOption:** This Object allows us to customize the behaviour of `callService` and has the following properties.

- `sendToRedux: boolean`: Setting this to true will send the response to redux. Enabling this option requires a Redux Store Setup. Look at [Bifrost Services](./bifrost_services.md) for more info on how to setup one.

- `responseToast: boolean`: Setting this to true will send a toast message ("Request Successful") when a response is recieved from the service.
- `noErrorToast: boolean`: Errors are toasted by default. To Prevent automatic toast messaging Errors, set this property to true.
- `successToastMessage: string`: Custom string to be used instead of "Request Successful" when `responseToast` is set to true.
- `errorToastMessage: string`: By Default, the error messages from toasts are directly from `rosbridge_server`. This can be overrided using this custom string message.
- `handleResponse: (response)=>void`: This is a callback function when the response is recieved from the server and can be used to do some stuff that can't be done using any of the other params on `CallServiceOption`.

### Additional Notes

- Multiple Bifrost Instances can exist on the same Component. This would be ideal if a single component is subscribing to multiple topics / services.
