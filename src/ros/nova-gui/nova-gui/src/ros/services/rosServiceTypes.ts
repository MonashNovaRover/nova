import { CameraOperationMessage } from "../rosTypes";
import { RosService } from "./rosServices";

interface RosServiceMessage<REQ, RES> {
  request: REQ;
  response?: RES;
}

export interface RosServiceInterface {
  [RosService.NULL_SERVICE]: RosServiceMessage<any, any>;
  [RosService.START_STREAM]: RosServiceMessage<CameraOperationMessage, any>;
  [RosService.GET_IP_LIST]: RosServiceMessage<any, string[]>;
}
