import humanizeString from "humanize-string";

interface PropRendererProps<T> {
  props: T;
  ignoreProps: Array<keyof T>;
}

/**
 * Renders the properties of an object.
 * @template T - The type of the object.
 * @param {PropRendererProps<T>} props - The props containing the object and other configuration options.
 * @param {PropRendererProps<T>} ignoreProps - The Props to Ignore while Rendering
 */
export const PropRenderer = <T extends object>(props: PropRendererProps<T>) => {
  return (
    <div className="w-full">
      {Object.keys(props.props)
        .filter((prop) => !props.ignoreProps.includes(prop as keyof T))
        .map((prop) => (
          <div className="flex flex-row justify-between">
            <div className="font-semibold ">{humanizeString(prop)}</div>
            <div>{String(props.props[prop as keyof T])}</div>
          </div>
        ))}
    </div>
  );
};
