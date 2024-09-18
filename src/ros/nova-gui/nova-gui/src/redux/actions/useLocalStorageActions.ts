import {useDispatch} from "react-redux";
import {LSKeyValuePair} from "../models/LocalStorageState.ts";
import {LocalStorageActions} from "../slices/LocalStorageSlice.ts";

export function useLocalStorageActions() {
    const dispatch = useDispatch();

    return {
        setValue(value: LSKeyValuePair) {
            dispatch({
                type: LocalStorageActions.SET_VALUE.toString(),
                payload: value,
            })
        }
    }
}
