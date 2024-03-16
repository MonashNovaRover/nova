import {useEffect, useRef} from "react";

/**
 * This custom hook help with the issues of constant objects causing an update, when nothing inside the object changes.
 * @param factory A function that produces a object as a dictionary
 * @param deps A list of all dependencies for generating the object, so the dictionary can update when anything inside
 * the object dictionary changes.
 */
export default function useDict<T>(factory: () => ({[key: string]: T}), deps?: React.DependencyList ) {
  const dict = useRef<{[key: string]: T}>({});

  useEffect(() => {
    dict.current = factory();
  }, deps);

  return dict.current;
}
