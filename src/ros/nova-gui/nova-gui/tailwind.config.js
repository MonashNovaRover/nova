/** @type {import('tailwindcss').Config} */
const { nextui } = require("@nextui-org/react");

export default {
  content: ["./index.html", "./src/**/*.{js,ts,jsx,tsx}", "./node_modules/@nextui-org/theme/dist/**/*.{js,ts,jsx,tsx}"],
  theme: {
    extend: {
      colors: {
        '292929': '#292929',
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
      spacing: {
      '142': '35.5rem',
      }
    },
  },
  darkMode: "class",
  plugins: [nextui()],
};
