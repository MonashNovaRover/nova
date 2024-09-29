import {useDispatch, useSelector} from "react-redux";
import {RootState} from "../RootState.ts";

export function useGenericStore<T>(storeName: string) {
  const value = useSelector((store: RootState) => store[storeName as keyof typeof store] as T | undefined)

  const dispatch = useDispatch();
  const setValue = (value: T) => {
    dispatch({
      type: storeName + "/SET_VALUE",
      payload: value,
    })
  }

  return [value, setValue]
}
