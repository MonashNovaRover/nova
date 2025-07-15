import React from "react";
import SegmentedPicker from "../../shared/components/SegmentedPicker/SegmentedPicker.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";

export interface SiteSelectWidgetProps {
  pickerClassName?: string,
}

const SiteSelectWidget: React.FC<SiteSelectWidgetProps> = (
  {
    pickerClassName,
  }) => {

  const [currentSite, setCurrentSite] = useGenericStore<Site>("currentSite");

  return (
    <SegmentedPicker
      fullWidth
      className={"grow " + (pickerClassName ?? "")}
      onIndexChange={setCurrentSite}
      selectedIndex={currentSite}
    >
      <div className="text-rose-300">Site 1</div>
      <div className="text-amber-200">Site 2</div>
      <div className="text-sky-300">Site 3</div>
      <div className="text-violet-300">Site 4</div>
    </SegmentedPicker>
  );
};

export default SiteSelectWidget;