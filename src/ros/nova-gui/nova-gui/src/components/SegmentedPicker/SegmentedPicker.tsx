import {Tab, Tabs, TabsProps} from "@nextui-org/react";
import React, {Children, Key} from "react";


interface SegmentedPickerProps extends TabsProps {
  selectedIndex?: number,
  onIndexChange?: number,
  children?: ReactChildren
}

/**
 * A component based on Next UI's Tabs component, but for choosing a value, rather than navigating between different
 * Tab components. Uses an index for the value.
 * @param props Same as TabsProps, but use selectedIndex instead of selectedKey and onIndexChange instead of
 * onSelectionChange. Children should be the labels for each option, not Tab elements.
 * @constructor
 */
const SegmentedPicker: React.FC<SegmentedPickerProps> = ({ children: children, ...props}) => {
  const onSelectionChange = props.onIndexChange ? (key: Key) => props.onIndexChange(+key) : undefined;
  const selectedKey = props.selectedIndex?.toString();

  const titles = Children.toArray(children);

  return (
    <Tabs {...props} onSelectionChange={onSelectionChange} selectedKey={selectedKey}>
      {Children.map(children, (child, index) =>
        <Tab key={index.toString()} title={child}/>
      )}
    </Tabs>
  )
}

export default SegmentedPicker;