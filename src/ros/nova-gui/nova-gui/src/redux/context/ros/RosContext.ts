import { createContext } from "react";

export const RosContext = createContext<ROSLIB.Ros | undefined>(undefined);
