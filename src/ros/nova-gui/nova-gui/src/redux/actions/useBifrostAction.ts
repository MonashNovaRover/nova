import { useDispatch } from "react-redux";
import { createBifrostAction } from "./createBifrostAction";
import { bindActionCreators } from "@reduxjs/toolkit";
import { RosTopics } from "../../ros/topics/rosTopics";
import { useContext } from "react";
import { RosContext } from "../context/RosContext";
import { RosService } from "../../ros/services/rosServices";

export interface BifrostProps {
  topic?: RosTopics;
  service?: RosService;
}

export const useBifrost = (props: BifrostProps) => {
  const ros = useContext(RosContext);
  const bifrostActions = createBifrostAction(props, ros);
  const dispatch = useDispatch();

  return bindActionCreators(bifrostActions, dispatch);
};
