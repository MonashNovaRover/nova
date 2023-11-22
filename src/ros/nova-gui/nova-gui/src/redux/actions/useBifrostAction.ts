import { useDispatch } from "react-redux";
import { createBifrostAction } from "./createBifrostAction";
import { bindActionCreators } from "@reduxjs/toolkit";
import { RosTopics } from "../../ros/topics";

export const useBifrostActions = (topic: RosTopics) => {
  const bifrostActions = createBifrostAction(topic);
  const dispatch = useDispatch();

  return bindActionCreators(bifrostActions, dispatch);
};
