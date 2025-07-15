import {useLocalStorage} from "../../../hooks/useLocalStorage.ts";
import {useCallback, useState} from "react";
import {isNumber} from "lodash";


export default function useNumberField(key: string, defaultValue: number): [number, string, (value: string) => void] {

  const [stringValue, setStringValue] = useLocalStorage(key, defaultValue.toString());
  const [numberValue, setNumberValue] = useState<number>(defaultValue);

  const setValue = useCallback((newValue: string) => {
    setStringValue(newValue);

    const parsedString = +newValue;
    if (!isNumber(parsedString))
      return;
    if (isNaN(parsedString))
      return;

    setNumberValue(parsedString);

  }, [setStringValue, setNumberValue])

  return [numberValue, stringValue, setValue];

}






