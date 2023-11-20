import { useDispatch } from "react-redux";
import { createBifrostAction } from "./createBifrostAction";
import { useMemo } from "react";
import { bindActionCreators } from "@reduxjs/toolkit";

export const useBifrostAction = <T>() => {
  const bifrostActions = createBifrostAction<T>();
  const dispatch = useDispatch();

  //   return useMemo(
  //     () => bindActionCreators(bifrostActions, dispatch),
  //     [bifrostActions, dispatch]
  //   );
  return bindActionCreators(bifrostActions, dispatch);
};
