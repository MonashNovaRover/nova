import { useDispatch } from "react-redux";
import { createBifrostAction } from "./createBifrostAction";
import { bindActionCreators } from "@reduxjs/toolkit";
import { RosTopics } from "../../ros/rosTopics";

export const useBifrost = (topic: RosTopics) => {
  const bifrostActions = createBifrostAction(topic);
  const dispatch = useDispatch();

  return bindActionCreators(bifrostActions, dispatch);
};
