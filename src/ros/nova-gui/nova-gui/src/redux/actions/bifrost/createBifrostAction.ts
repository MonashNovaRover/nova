import { Ros, Service, Topic } from "roslib";
import { BifrostConnectionStatus } from "../../models/bifrost/BifrostTypes";
import { RootState } from "../../RootState";
import { RosTopicInterfaces } from "../../../ros/topics/rosTopicTypes";
import { RosTopic } from "../../../ros/topics/rosTopic";
import { rosTopicMessages } from "../../../ros/topics/rosTopicMessages";
import { BifrostProps } from "./useBifrostAction";
import { RosService } from "../../../ros/services/rosService";
import { rosServiceMessages } from "../../../ros/services/rosServiceMessages";
import { RosServiceInterface } from "../../../ros/services/rosServiceTypes";
import toast from "react-hot-toast";

export enum BifrostActionTypes {
  UPDATE_TOPIC_STATE = "UPDATE_TOPIC_STATE",
  UPDATE_SERVICE_STATE = "UPDATE_SERVICE_STATE",
  INITIATE_CONTACT = "INITIATE_CONTACT",
  CONNECTION_UPDATE = "CONNECTION_UPDATE",
  SUBSCRIBE_TOPIC = "SUBSCRIBE_TOPIC",
}

export interface BifrostActionType<P> {
  type: string;
  payload: P;
}

type CustomDispatch<P> = (action: () => BifrostActionType<P>) => void;

interface CallServiceOptions<T> {
  sendToRedux?: boolean;
  responseToast?: boolean;
  noErrorToast?: boolean;
  successToastMessage?: string;
  errorToastMessage?: string;
  handleResponse?: (response: T) => void;
}

export function createBifrostAction(props: BifrostProps, ros?: Ros) {
  const { topic = RosTopic.NULL_TOPIC, service = RosService.NULL_SERVICE } =
    props;

  return {
    // Private Methods Below. DO NOT USE. I know this isin't a class but helps to keep it private
    _getTopicAction(
      object: RosTopicInterfaces[typeof topic]
    ): () => BifrostActionType<RosTopicInterfaces[typeof topic]> {
      return () => ({
        type:
          BifrostActionTypes.UPDATE_TOPIC_STATE.toString() + topic.toString(),
        payload: { ...object } as RosTopicInterfaces[typeof topic],
      });
    },
    _getServiceAction(
      object: RosServiceInterface[typeof service]
    ): () => BifrostActionType<RosServiceInterface[typeof service]> {
      return () => ({
        type:
          BifrostActionTypes.UPDATE_SERVICE_STATE.toString() +
          "/" +
          service.toString(),
        payload: { ...object } as RosServiceInterface[typeof service],
      });
    },
    _updateBifrostConnectionStatus(connectionStatus: BifrostConnectionStatus) {
      return () => ({
        type: BifrostActionTypes.CONNECTION_UPDATE,
        payload: connectionStatus,
      });
    },
    _updateSubscribedTopics(topic: RosTopic) {
      return () => ({
        type: BifrostActionTypes.SUBSCRIBE_TOPIC,
        payload: topic,
      });
    },
    _updateTopicState(object: RosTopicInterfaces[typeof topic]) {
      return (dispatch: CustomDispatch<RosTopicInterfaces[typeof topic]>) => {
        dispatch(
          this._getTopicAction(object as RosTopicInterfaces[typeof topic])
        );
      };
    },
    _updateServiceState(
      object: RosServiceInterface[typeof service]["response"]
    ) {
      return (
        dispatch: CustomDispatch<
          RosServiceInterface[typeof service]["response"]
        >
      ) => {
        dispatch(
          this._getServiceAction(object as RosServiceInterface[typeof service])
        );
      };
    },
    updateBifrostConnection(connectionStatus: BifrostConnectionStatus) {
      return (dispatch: CustomDispatch<BifrostConnectionStatus>) => {
        dispatch(this._updateBifrostConnectionStatus(connectionStatus));
      };
    },
    // Public Methods
    /**
     * Synchronizes with a topic by subscribing to it and updating the topic state when a message is received.
     */
    syncWithTopic() {
      return (
        dispatch: CustomDispatch<RosTopic>,
        getState: () => RootState
      ) => {
        const state = getState();
        if (
          state.bifrostStatus.subscribedTopics.includes(topic) ||
          !ros ||
          topic === RosTopic.NULL_TOPIC
        )
          return;

        const rosTopic = new Topic({
          ros: ros,
          name: topic.toString(),
          messageType: rosTopicMessages[topic],
        });

        rosTopic.subscribe(() => {});

        rosTopic.on("message", (message: RosTopicInterfaces[typeof topic]) => {
          this._updateTopicState(message);
        });

        dispatch(this._updateSubscribedTopics(topic));
      };
    },
    /**
     * Calls a ROS service with the provided request and options.
     * @param request The request object for the service call.
     * @param options The options for the service call.
     * @returns None
     */
    callService(
      request: RosServiceInterface[typeof service]["request"],
      options: CallServiceOptions<
        RosServiceInterface[typeof service]["response"]
      > = {
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
            if (!resp) return;
            if (options.handleResponse) options.handleResponse(resp);
            if (options.responseToast || options.successToastMessage) {
              const toastMessage =
                options.successToastMessage ?? "Request Successful";
              toast.success(toastMessage);
            }

            if (options.sendToRedux) {
              this._updateServiceState(
                resp as RosServiceInterface[typeof service]["response"]
              );
            }
          },
          (error) => {
            if (!options.noErrorToast || options.errorToastMessage) {
              const errorToastMessage =
                options.errorToastMessage ?? `Request Failed: ${error}`;
              toast.error(errorToastMessage);
            }
          }
        );
      };
    },
    callServiceToRedux(
      request: RosServiceInterface[typeof service]["request"],
      options: CallServiceOptions<
        RosServiceInterface[typeof service]["response"]
      > = {
        noErrorToast: false,
        responseToast: true,
      }
    ) {
      this.callService(request, { ...options, sendToRedux: true });
    },
    /**
     * Publishes the message to the ros topic.
     * @param message The message to publish on the ros topic
     * @returns None
     */
    publishToTopic(message: RosTopicInterfaces[typeof topic]) {
      return () => {
        if (!ros || topic === RosTopic.NULL_TOPIC || message === undefined)
          return;

        const rosTopic = new Topic({
          ros: ros,
          name: topic.toString(),
          messageType: rosTopicMessages[topic],
        });

        rosTopic.publish(message);
      };
    },
};
}
