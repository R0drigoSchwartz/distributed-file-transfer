from fastapi import FastAPI

app = FastAPI()


@app.get("/files")
async def get_file_status():
    file_status = {}
    try:
        with open("status_file.txt", "r", encoding="utf-8") as file:
            for line in file:
                path, _, status = line.split()
                file_status[path] = "Disponível" if status == "complete" else "Em transferência" 

        return file_status
    except FileNotFoundError:
        return {}