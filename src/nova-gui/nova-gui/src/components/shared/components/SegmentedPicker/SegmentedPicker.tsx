import {Tab, Tabs, TabsProps} from "@nextui-org/react";
import React, {Children, Key, ReactNode} from "react";

export interface SegmentedPickerProps extends TabsProps {
  selectedIndex?: number,
  onIndexChange?: ((index: number) => void),
  children: ReactNode
}

// Converts Key to number. We eliminate bigint, as no SegmentedPicker should have enough items to justify it.
const keyToNumber: (key: Key) => number = (key) => {
  const value = key.valueOf();

  if (typeof value === "bigint")
    return Number(value);
  return +value;
}

/**
 * A component based on Next UI's Tabs component, but for choosing a value, rather than navigating between different
 * Tab components. Uses an index for the value.
 * @param props Same as TabsProps, but use selectedIndex instead of selectedKey and onIndexChange instead of
 * onSelectionChange. Children should be the labels for each option, not Tab elements.
 * @constructor
 */
const SegmentedPicker: React.FC<SegmentedPickerProps> = ({
  children: children,
  onIndexChange: onIndexChange,
  selectedIndex: selectedIndex,
  ...props
}) => {
  const onSelectionChange = !onIndexChange ? undefined
    : (key: Key) => onIndexChange!(keyToNumber(key))
  const selectedKey = selectedIndex !== undefined ? (selectedIndex)?.toString() : undefined;

  const childrenArray = Children.toArray(children);

  const tabs = childrenArray.map((child, index) => (
    <Tab key={index.toString()} title={child}>{}</Tab>
  ));

  return (
    <Tabs onSelectionChange={onSelectionChange} selectedKey={selectedKey} {...props}>
      {tabs}
    </Tabs>
  )
}
// {...props} onSelectionChange={onSelectionChange} selectedKey={selectedKey}

export default SegmentedPicker;