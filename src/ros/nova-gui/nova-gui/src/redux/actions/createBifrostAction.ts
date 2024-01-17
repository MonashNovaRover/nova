import { Ros, Service, Topic } from "roslib";
import { BifrostConnectionStatus } from "../models/BifrostTypes";
import { RootState } from "../RootState";
import { RosTopicInterfaces } from "../../ros/topics/rosTopicTypes";
import { RosTopics } from "../../ros/topics/rosTopics";
import { rosTopicMessages } from "../../ros/topics/rosTopicMessages";
import { BifrostProps } from "./useBifrostAction";
import { RosService } from "../../ros/services/rosServices";
import { rosServiceMessages } from "../../ros/services/rosServiceMessages";
import { RosServiceInterface } from "../../ros/services/rosServiceTypes";
import toast from "react-hot-toast";

export enum BifrostActionTypes {
  UPDATE_DATA = "UPDATE_DATA_",
  UPDATE_SERVICE_DATA = "UPDATE_SERVICE_DATA",
  INITIATE_CONTACT = "INITIATE_CONTACT",
  CONNECTION_UPDATE = "CONNECTION_UPDATE",
  SUBSCRIBE_TOPIC = "SUBSCRIBE_TOPIC",
}

export interface BifrostActionType<P> {
  type: string;
  payload: P;
}

type CustomDispatch<P> = (action: () => BifrostActionType<P>) => void;

interface CallServiceOptions {
  sendToRedux?: boolean;
  responseToast?: boolean;
  noErrorToast?: boolean;
  successToastMessage?: string;
  errorToastMessage?: string;
  handleResponse?: (response: any) => void;
}

export function createBifrostAction(props: BifrostProps, ros?: Ros) {
  const { topic = RosTopics.NULL_TOPIC, service = RosService.NULL_SERVICE } =
    props;

  return {
    _update(
      object: RosTopicInterfaces[typeof topic]
    ): () => BifrostActionType<RosTopicInterfaces[typeof topic]> {
      return () => ({
        type: BifrostActionTypes.UPDATE_DATA.toString() + topic.toString(),
        payload: { ...object } as RosTopicInterfaces[typeof topic],
      });
    },
    _updateBifrostConnectionStatus(connectionStatus: BifrostConnectionStatus) {
      return () => ({
        type: BifrostActionTypes.CONNECTION_UPDATE,
        payload: connectionStatus,
      });
    },
    _updateSubscribedTopics(topic: RosTopics) {
      return () => ({
        type: BifrostActionTypes.SUBSCRIBE_TOPIC,
        payload: topic,
      });
    },
    _updateState(object: RosTopicInterfaces[typeof topic]) {
      return (dispatch: CustomDispatch<RosTopicInterfaces[typeof topic]>) => {
        dispatch(this._update(object as RosTopicInterfaces[typeof topic]));
      };
    },
    _updateServiceState(object: RosServiceInterface[typeof service]) {
      return (
        dispatch: CustomDispatch<RosServiceInterface[typeof service]>
      ) => {
        dispatch(() => ({
          type:
            BifrostActionTypes.UPDATE_SERVICE_DATA.toString +
            service.toString(),
          payload: { ...object } as RosServiceInterface[typeof service],
        }));
      };
    },
    updateBifrostConnection(connectionStatus: BifrostConnectionStatus) {
      return (dispatch: CustomDispatch<BifrostConnectionStatus>) => {
        dispatch(this._updateBifrostConnectionStatus(connectionStatus));
      };
    },
    syncWithTopic() {
      return (
        dispatch: CustomDispatch<RosTopics>,
        getState: () => RootState
      ) => {
        const state = getState();
        if (
          state.bifrostStatus.subscribedTopics.includes(topic) ||
          !ros ||
          topic === RosTopics.NULL_TOPIC
        )
          return;

        const rosTopic = new Topic({
          ros: ros,
          name: topic.toString(),
          messageType: rosTopicMessages[topic],
        });

        rosTopic.subscribe(() => {
        });

        rosTopic.on("message", (message: RosTopicInterfaces[typeof topic]) => {
          this._updateState(message);
        });

        dispatch(this._updateSubscribedTopics(topic));
      };
    },
    callService(
      request: RosServiceInterface[typeof service]["request"],
      options: CallServiceOptions = {
        sendToRedux: false,
        noErrorToast: false,
        responseToast: true,
      }
    ) {
      return () => {
        if (!ros || service === RosService.NULL_SERVICE) return;

        const rosService = new Service({
          ros: ros,
          name: service.toString(),
          serviceType: rosServiceMessages[service],
        });

        rosService.callService(
          request,
          (resp: RosServiceInterface[typeof service]["response"]) => {
            if (options.handleResponse) options.handleResponse(resp);
            if (options.responseToast) {
              const toastMessage =
                options.successToastMessage ?? "Request Successful";
              toast.success(toastMessage);
            }

            if (options.sendToRedux) {
              this._updateServiceState(resp);
            }
          },
          (error) => {
            if (!options.noErrorToast) {
              const errorToastMessage =
                options.errorToastMessage ?? `Request Failed: ${error}`;
              toast.error(errorToastMessage);
            }
          }
        );
      };
    },
  };
}
