export const isIPAddress = (value: string) => {
  const ipRegex = /^([0-9]{1,3}\.){3}[0-9]{1,3}$/;
  return ipRegex.test(value);
};
