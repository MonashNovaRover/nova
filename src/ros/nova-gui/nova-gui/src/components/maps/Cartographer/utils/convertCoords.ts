import { useGenericStore } from "../../../../hooks/useGenericStore.ts";
import { MapCoordinate} from "../../../../redux/models/CartographerState.ts";

export type DisplayMapCoordinate = {
    lat: string,
    long: string
}

export type DD = {
    degrees: number
}

export type DMS = {
    degrees: number
    minutes: number
    seconds: number
    direction: string
}

export type DDM = {
    degrees: number
    minutes: number
    direction: string
}

export function DDtoDMS(dd: number, isLat: boolean | undefined = undefined): DMS {
    const absDD = Math.abs(dd);
    const degrees = Math.floor(absDD);
    const minutes = Math.floor((absDD - degrees) * 60);
    const seconds = ((absDD - degrees - minutes / 60) * 3600).toFixed(2);

    return {
        degrees: degrees,
        minutes: minutes,
        seconds: parseFloat(seconds),
        direction: isLat === undefined 
            ? (dd >= 0 ? "N/E" : "S/W") 
            : (isLat 
                ? (dd >= 0 ? "N" : "S") 
                : (dd >= 0 ? "E" : "W"))
    };
}

export function DDtoDDM(dd: number, isLat: boolean | undefined = undefined): DDM {
    const absDD = Math.abs(dd);
    const degrees = Math.floor(absDD);
    const minutes = (absDD - degrees) * 60;

    return {
        degrees: degrees,
        minutes: minutes,
        direction: isLat === undefined 
            ? (dd >= 0 ? "N/E" : "S/W") 
            : (isLat 
                ? (dd >= 0 ? "N" : "S") 
                : (dd >= 0 ? "E" : "W"))
    };
}

export function DMStoDD(dms: DMS): number {
    const dd = dms.degrees + (dms.minutes / 60) + (dms.seconds / 3600);
    const finalDD = dms.direction.toUpperCase() === 'S' || dms.direction.toUpperCase() === 'W' ? dd * -1 : dd;
    return finalDD
}

export function DDMtoDD(ddm: DDM): number {
    const dd = ddm.degrees + (ddm.minutes / 60);
    const finalDD = ddm.direction.toUpperCase() === 'S' || ddm.direction.toUpperCase() === 'W' ? dd * -1 : dd;
    return finalDD
}

export function calcLatFromDD(lat: string): number {
    if (lat.slice(-1) === "°") return +lat.slice(0,-1);
    return +lat
}

export function calcLongFromDD(long: string): number {
    if (long.slice(-1) === "°") return +long.slice(0,-1);
    return +long
}


export function calcLatFromDMS(lat: string): number {
    const latArr = lat.split(" ").map((v, i, a)=>a.length-1 !== i ? (v.slice(-1) === ["°", "'", "\""][i] ? +v.slice(0,-1) : +v) : v);
    if (latArr.length !== 4) return NaN
    const latDMS = {degrees: latArr[0], minutes: latArr[1], seconds: latArr[2], direction: latArr[3]} as DMS;
    return DMStoDD(latDMS);
}

export function calcLongFromDMS(long: string): number {
    const longArr = long.split(" ").map((v, i, a)=>a.length-1 !== i ? (v.slice(-1) === ["°", "'", "\""][i] ? +v.slice(0,-1) : +v) : v);
     if (longArr.length !== 4) return NaN
    const longDMS = {degrees: longArr[0], minutes: longArr[1], seconds: longArr[2], direction: longArr[3]} as DMS;
    return DMStoDD(longDMS);
}


export function calcLatFromDDM(lat: string): number {
    const latArr = lat.split(" ").map((v, i, a)=>a.length-1 !== i ? (v.slice(-1) === ["°", "'"][i] ? +v.slice(0,-1) : +v) : v);
     if (latArr.length !== 3) return NaN
    const latDMS = {degrees: latArr[0], minutes: latArr[1], direction: latArr[2]} as DDM;
    return DDMtoDD(latDMS)
}

export function calcLongFromDDM(long: string): number {
    const longArr = long.split(" ").map((v, i, a)=>a.length-1 !== i ? (v.slice(-1) === ["°", "'"][i] ? +v.slice(0,-1) : +v) : v);
     if (longArr.length !== 3) return NaN
    const longDMS = {degrees: longArr[0], minutes: longArr[1], direction: longArr[2]} as DDM;
    return DDMtoDD(longDMS)
}


export function useCalcMapCoordinate(coord: DisplayMapCoordinate): MapCoordinate {
    const [cartographerCoordinateFormat] = useGenericStore<number>("cartographerCoordinateFormat");
    switch (cartographerCoordinateFormat) {
        /** numbers align with coordinateFormatOptions, there should be a better way to do this from "../../../navbar/settings/CartographerSettings.tsx"*/
        case 0:
            return {lat: calcLatFromDD(coord.lat), long: calcLongFromDD(coord.long)};
        case 1:
            return {lat: calcLatFromDMS(coord.lat), long: calcLongFromDMS(coord.long)};
        case 2:
            return {lat: calcLatFromDDM(coord.lat), long: calcLongFromDDM(coord.long)};
        default:
            return {lat: calcLatFromDD(coord.lat), long: calcLongFromDD(coord.long)};
    };
}


export function displayMapCoordinateAsDD({lat, long}: MapCoordinate): DisplayMapCoordinate {
    return {lat: lat.toString()+"°", long: long.toString()+"°"} as DisplayMapCoordinate
}

export function displayMapCoordinateAsDMS({lat, long}: MapCoordinate): DisplayMapCoordinate {
    const dmsLat = DDtoDMS(lat, true);
    const dmsLong = DDtoDMS(long, false);
    return {
        lat: `${dmsLat.degrees}° ${dmsLat.minutes}' ${dmsLat.seconds}" ${dmsLat.direction}`,
        long: `${dmsLong.degrees}° ${dmsLong.minutes}' ${dmsLong.seconds}" ${dmsLong.direction}`
    }
}

export function displayMapCoordinateAsDDM({lat, long}: MapCoordinate): DisplayMapCoordinate {
    const dmsLat = DDtoDDM(lat, true);
    const dmsLong = DDtoDDM(long, false);
    return {
        lat: `${dmsLat.degrees}° ${dmsLat.minutes}' ${dmsLat.direction}`,
        long: `${dmsLong.degrees}° ${dmsLong.minutes}' ${dmsLong.direction}`
    }
}

export function useDisplayMapCoordinate(coord: MapCoordinate): DisplayMapCoordinate {
    const [cartographerCoordinateFormat] = useGenericStore<number>("cartographerCoordinateFormat");
    switch (cartographerCoordinateFormat) {
        /** numbers align with coordinateFormatOptions, there should be a better way to do this from "../../../navbar/settings/CartographerSettings.tsx"*/
        case 0:
            return displayMapCoordinateAsDD(coord);
        case 1:
            return displayMapCoordinateAsDMS(coord);
        case 2:
            return displayMapCoordinateAsDDM(coord);
        default:
            return displayMapCoordinateAsDD(coord);
    };
}

export function displayMapCoordinate(coord: MapCoordinate, cartographerCoordinateFormat: number): DisplayMapCoordinate {
    switch (cartographerCoordinateFormat) {
        /** numbers align with coordinateFormatOptions, there should be a better way to do this from "../../../navbar/settings/CartographerSettings.tsx"*/
        case 0:
            return displayMapCoordinateAsDD(coord);
        case 1:
            return displayMapCoordinateAsDMS(coord);
        case 2:
            return displayMapCoordinateAsDDM(coord);
        default:
            return displayMapCoordinateAsDD(coord);
    };
}