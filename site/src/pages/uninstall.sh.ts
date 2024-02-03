import fs from "node:fs/promises";

const url = new URL("../../../uninstall.sh", import.meta.url);
const script = await fs.readFile(url, "utf-8");

export async function GET() {
  return new Response(script);
}
