/** @type {import('tailwindcss').Config} */
import { nextui } from "@nextui-org/react";

export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
    "./node_modules/@nextui-org/theme/dist/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      spacing: {
      '154': '38.5rem',
      },    
      colors: {
        292929: "#292929",
        primary: "#3f3fe6",
        secondary: "#cc2988",
        success: "#2ac776",
        warning: "#f57720",
        danger: "#f2312d"
        // TODO: Fix issues with different modes
        // light: {
        //   primary: "#FFFFFF",
        //   secondary: "#E5E7EB",
        // },
        // dark: {
        //   primary: "#1F2937",
        //   secondary: "#1A1A1A",
        //   bkground: "#131313",
        //   navbkground: "#1A1A1A",
        // },
      },
    },
  },
  darkMode: "class",
  plugins: [nextui()],
};
