import {FC, HTMLAttributes, ReactNode} from "react";

export interface OverlayProps extends HTMLAttributes<HTMLDivElement> {
  overlay?: ReactNode,

  // Additional classes for the inner div that wraps the given children
  innerClassName?: string,
  // Additional classes for the inner div that wraps the given overlay
  overlayClassName?: string
  // Removes default styling that centres the given overlay
  noCentering?: boolean
}

/**
 * A component that wraps children with a div that overlays the element in `props.overlay` on top of those elements.
 * @param props
 * @constructor
 */
const Overlay: FC<OverlayProps> = (props) => {

  const wrapperClassName = "relative overflow-hidden " + props.className;
  const overlayClassName = "absolute top-0 left-0 right-0 bottom-0 overflow-hidden pointer-events-none "
    + (props.noCentering ? "" : "flex justify-evenly justify-items-center ")
    + (props.overlayClassName ?? "");
  const innerClassName = "relative " + (props.innerClassName);

  return (
    <div {...props} className={wrapperClassName}>
      { props.children &&
        <div className={innerClassName}>
          {props.children}
        </div>
      }
      { props.overlay &&
        <div className={overlayClassName}>
          {props.overlay}
        </div>
      }
    </div>
  );
};

export default Overlay;