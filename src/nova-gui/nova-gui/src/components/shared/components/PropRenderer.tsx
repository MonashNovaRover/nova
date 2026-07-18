import humanizeString from "humanize-string";
import CopyableInput from "./CopyableInput/CopyableInput.tsx";

interface PropRendererProps<T> {
  props: T;
  ignoreProps: Array<keyof T>;
  row?: boolean;
  size?: "sm" | "md" | "lg";
}

/**
 * Renders the properties of an object.
 * @template T - The type of the object.
 * @param {PropRendererProps<T>} props - The props containing the object and other configuration options.
 * @param {PropRendererProps<T>} ignoreProps - The Props to Ignore while Rendering
 */
export const PropRenderer = <T extends object>(props: PropRendererProps<T>) => {
  return (
    <div
      className={"w-full flex" + (props.row ? " flex-row gap-1" : " flex-col ")}
    >
      {Object.keys(props.props)
        .filter((prop) => !props.ignoreProps.includes(prop as keyof T))
        .map((prop, i) => {
          return (
            <CopyableInput
              key={i}
              readOnly
              variant="faded"
              label={humanizeString(prop)}
              value={String(props.props[prop as keyof T])}
              className="w-full my-1"
              size={props.size}
            ></CopyableInput>
          );
        })}
    </div>
  );
};
