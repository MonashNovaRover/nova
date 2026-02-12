import { createContext } from "react";
import * as ROSLIB from "roslib";

export const RosContext = createContext<ROSLIB.Ros | undefined>(undefined);
