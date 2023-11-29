import { useDispatch } from "react-redux";
import { createBifrostAction } from "./createBifrostAction";
import { bindActionCreators } from "@reduxjs/toolkit";
import { RosTopics } from "../../ros/rosTopics";
import { useContext } from "react";
import { RosContext } from "../../RosRoot";

export const useBifrost = (topic: RosTopics) => {
  const ros = useContext(RosContext);
  const bifrostActions = createBifrostAction(topic, ros);
  const dispatch = useDispatch();

  return bindActionCreators(bifrostActions, dispatch);
};
