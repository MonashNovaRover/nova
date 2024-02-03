### Bifrost Services

For Bifrost to send Service Requests to ROS, Bifrost should be aware of the services that it can call. The `services` under the `ros` directory contains essential information such as Service name, Message Type (in TS and ROS), etc. In general, GUI cares about 2 types of services.

- The LetHimCook Services: These services are called but no response is expected or anticipated (unless it is an error of some sort). We just don't care about the Respose and we let him cook.

- The MakeMeASandwich Service: This service is called with the hope of receiving something return. We patiently wait for the response, hoping that the sandwich-making magic happens behind the scenes. And when the Response is recieved, we use the Params from `CallServiceOptions` to do something about it.

For Demonstration Purposes, we shall add a service `servo/set_servo` (Servo for context is a motor that has position control. We are trying to Set tge Position of the Servo)

---

Service Name: `servo/set_servo`

Service Type (ROS): `core/srv/servo`

```py
# core/srv/servo.srv
float64 position
---
bool success
```

---

The following files should be modified inside the `services` directory under the `ros` directory.

##### [`rosService.ts`](../nova-gui/src/ros/services/rosService.ts)

This File exports a single enum `RosService` which is used to point Bifrost at the service of interest. To add the service above, we add the following to `RosService`.

```ts
SET_SERVO = "servo/set_servo",
```

##### [`rosServiceMessages.ts`](../nova-gui/src/ros/services/rosServiceMessages.ts)

This file exports an object that links Services found on `RosService.ts` to the message types defined in ROS.This is to ensure that `roslibjs` and `rosbridge_server` are aware of what type of Service is being called. In this case, we add the following to the object

```ts
export const rosServiceMessages = {
  // Previous Types
  [RosService.SET_SERVO]: "core/srv/servo",
};
```

##### [`rosServiceTypes.ts`](../nova-gui/src/ros/services/rosServiceTypes.ts)

This file exports an object that links Services found on `rosService.ts` to the message types defined in Nova-GUI on `ros/rosTypes.ts`. In this case, we add the following to the object

```ts
export interface RosServiceInterfaces {
  // Previous Interfaces
  [RosService.SET_SERVO]: ROSServiceMessage<number, boolean>;
}
```

The `ROSServiceMessage<REQ,RES>` exists in the same file to help format these message types to that found in rclpy (message.request and message.response). In our case, `REQ` is a number (`float64`) and `RES` is a boolean from the `.srv` file.

The above steps are enough to add in A LetHimCook sort of service. It is also enough for services which don't send any responses to redux. If this option is enabled through bifrost. The steps below about creating a store for the responses is needed. For more info on enabling this option, read the [main bifrost](./bifrost.md#bifrostcallservice) doc.

### Bifrost Store

Data from `rosbridge_server` is stored in redux to be used by the entire app and the singleton object which stores all states is refered as `RootState`. `RootState` contains latest data from all subscribed topics and services and has additional elements to handle regular redux logic.

For creating a Store for the service. The Following changes should be made to files inside the `redux` directory.

##### [`RootState.ts`](../nova-gui/src/redux/RootState.ts)

This file contains the Structure of the RootState of the app discussed above. Data from the Service should Ideally be stored in `servoSuccessStore` under `RootState`.

```ts
export interface RootState {
  // Previous States
  servoSuccessStore: boolean;
}
```

Remember, this is just the structure. Creating the store is described below.

##### [`RootReducer.ts`](../nova-gui/src/redux/RootState.ts)

This file contains the Reducers for the stores and resembles states on `RootState`. For creating a store for `servoSuccessStore`, we use the function `createBifrostStore(<BifrostProps>,<Initial State>)`

```ts
export const rootReducer = {
  // Previous Stores
  servoSuccessStore: createBifrostStore(
    { service: RosService.SET_SERVO },
    false
  ),
};
```

Initial State is the data that is displayed till real data comes through `rosbridge_server`.

### Bifrost Usage

To Call Services through Bifrost, we make use of the hooks `useBifrost` and `useSelector`.`useSelector` is from `redux-toolkit` and is used to read values from the store.`useBifrost` invokes Bifrost and points it towards the selected service (kinda like [this](https://youtu.be/GDsWg_kxfTU?si=RXVpmboKSlbIOMct&t=54)).

```typescript
// Accessing the Store using useSelector hook
const servoSuccess = useSelector((state: Rootstate) => state.servoSuccessStore);

// Invoking Bifrost and pointing it towards SET_SERVO
const bifrost = useBifrost({ service: RosService.SET_SERVO });

const setServo = (position: number) => bifrost.callService(position);
const setServo = (position: number) => bifrost.callServiceToRedux(position); // Use This to set sendToRedux to true by default
```

`callService` has 2 arguments
**Request:** (Required) The Request that is to be sent to ROS. This is a regular Object whose type is configured in [`rosServiceTypes.ts`](../nova-gui/src/ros/services/rosServiceTypes.ts).

**CallServiceOption:** (Optional) This Object allows us to customize the behaviour of `callService` and has the following properties.

- `sendToRedux: boolean`: Setting this to true will send the response to redux. Enabling this option requires a Redux Store Setup. Look at [Bifrost Services](./bifrost_services.md) for more info on how to setup one.

- `responseToast: boolean`: Setting this to true will send a toast message ("Request Successful") when a response is recieved from the service.
- `noErrorToast: boolean`: Errors are toasted by default. To Prevent automatic toast messaging Errors, set this property to true.
- `successToastMessage: string`: Custom string to be used instead of "Request Successful" when `responseToast` is set to true.
- `errorToastMessage: string`: By Default, the error messages from toasts are directly from `rosbridge_server`. This can be overrided using this custom string message.
- `handleResponse: (response)=>void`: This is a callback function when the response is recieved from the server and can be used to do some stuff that can't be done using any of the other params on `CallServiceOption`.

An Alias function `callServiceToRedux` calls `callService` with the `sendToRedux` flag enabled by default, and is recommended for use cases with Redux.

The `callServiceOption` param is set to this default object

```ts
{
    sendToRedux: false,
    noErrorToast: false,
    responseToast: true,
}

```
