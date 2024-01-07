import {
  IRosStdSrvsSetBoolRequest,
  IRosStdSrvsSetBoolResponse,
} from "../rosTypes";
import { RosService } from "./rosServices";

interface RosServiceMessage<REQ, RES> {
  request: REQ;
  response?: RES;
}

export interface RosServiceInterface {
  [RosService.START_STREAM]: RosServiceMessage<
    IRosStdSrvsSetBoolRequest,
    IRosStdSrvsSetBoolResponse
  >;
}
