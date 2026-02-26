import {useNIRSiteData} from "../useNIRSiteData.ts";
import {Button} from "@nextui-org/react";
import {Download} from "react-feather";
import {NIRProbeReadingType} from "../SpaceResourcesSiteType.tsx";
import {useCallback} from "react";

/**
 * Button to download nir readings to a csv file
 * @constructor
 */
export const NIRProbeCSVDownloadButton = () => {
  const [readings, _] = useNIRSiteData();

  const downloadCSV = useCallback(() => {
    const headers = ["label", "type", "data"];

    const allReadings = [
      ...readings[NIRProbeReadingType.PD1],
      ...readings[NIRProbeReadingType.PD2],
    ];

    const csvRows = allReadings.map(row => formatRow(row, headers));

    const csvContent = [headers.join(","), ...csvRows].join("\n");

    const blob = new Blob([csvContent], { type: "text/csv;charset=utf-8;" });
    const url = URL.createObjectURL(blob);

    const link = document.createElement("a");
    link.href = url;
    const timestamp = new Date()
      .toISOString()
      .replace(/[:.]/g, "-");
    link.download = `nir-probe-data_${timestamp}.csv`;
    link.click();
  }, [readings])

  return (
    <Button
      isIconOnly
      variant={"light"}
      onPressStart={downloadCSV}>
      <Download/>
    </Button>
  )

}

// Formats the row based on the headers.
const formatRow = (row, headers) =>
  headers
    .map(field => {
      let value = row[field] ?? "";

      // Remove "auto_" prefix from label only
      if (field === "label" && typeof value === "string") {
        value = value.replace(/^auto_/, "");
      }

      if (typeof value === "object") {
        value = JSON.stringify(value);
      }

      return `"${String(value).replace(/"/g, '""')}"`;
    })
    .join(",")
