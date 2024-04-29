import humanizeString from "humanize-string";
import CopyableInput from "../CopyableInput/CopyableInput";

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
        .map((prop, i) => {
          return (
            <div className="flex flex-row justify-between gap-1" key={i}>
              <CopyableInput
                readOnly
                variant="faded"
                label={humanizeString(prop)}
                value={String(props.props[prop as keyof T])}
                className="w-full my-1"
              ></CopyableInput>
            </div>
          );
        })}
    </div>
  );
};
