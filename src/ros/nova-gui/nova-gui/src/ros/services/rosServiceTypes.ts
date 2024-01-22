import { RosService } from "./rosService";

interface RosServiceMessage<REQ, RES> {
  request: REQ;
  response?: RES;
}

export interface RosServiceInterface {
  [RosService.NULL_SERVICE]: RosServiceMessage<any, any>;
}
