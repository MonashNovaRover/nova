import { Reducer } from "@reduxjs/toolkit";
import {
  BifrostActionType,
  BifrostActionTypes,
} from "../../actions/bifrost/createBifrostAction";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { RosTopicInterfaces } from "../../../ros/topics/rosTopicTypes";
import { RosService } from "../../../ros/services/rosService";
import { RosServiceInterface } from "../../../ros/services/rosServiceTypes";
import { BifrostProps } from "../../actions/bifrost/useBifrostAction";
import { StoreContext, StoreType } from "../../models/StoreContext.ts";

export const createCustomReducer = <S, H extends object>(
  initialState: S,
  handlers: H
) => {
  const reducer = (state: S = initialState, action: { type: keyof H }): S => {
    if (Object.prototype.hasOwnProperty.call(handlers, action.type)) {
      return (
        handlers[action.type] as (state: S, action: { type: keyof H }) => S
      )(state, action);
    } else {
      return state;
    }
  };
  return reducer as Reducer<S>;
};

export const createBifrostStore = (
  props: BifrostProps,
  initialState: object
): StoreContext => {
  const { service = RosService.NULL_SERVICE, topic = RosTopic.NULL_TOPIC } =
    props;

  type TopicType = RosTopicInterfaces[typeof topic];

  type ServiceType = RosServiceInterface[typeof service];

  const reducerFunctions = {
    [BifrostActionTypes.UPDATE_TOPIC_STATE + topic]: (
      _: TopicType,
      action: BifrostActionType<TopicType>
    ) => {
      return {
        ...action.payload,
      };
    },
    [BifrostActionTypes.UPDATE_SERVICE_STATE + "/" + service]: (
      _: ServiceType,
      action: BifrostActionType<ServiceType>
    ) => {
      return {
        ...action.payload,
      };
    },
  };

  const reducer = createCustomReducer<object, typeof reducerFunctions>(
    initialState,
    reducerFunctions
  );

  return {
    storeType: StoreType.BIFROST,
    initialValue: initialState,
    reducer: reducer,
    shouldPersist: false,
    shouldTabSync: false,
  } as StoreContext
};
