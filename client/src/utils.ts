/// int16 only
export const numberToBytes = (n: number) =>
  n
    .toString(16)
    .padStart(4, "0")
    .match(/[a-f0-9]{2}/g)
    .reverse() // little endian
    .map((e) => parseInt(e, 16));

// lol better use padStart xd which also fixes problem with odd number of hex chars
//.concat([0, 0])
//.filter((_, i) => i < 2);
