# Authors: Rodrigo Schwartz (R0drigoSchwartz) and Vinicius Henrique Ribeiro (vini-ribeiro)

from fastapi import FastAPI

app = FastAPI()


@app.get("/files")
async def get_file_status():
    file_status = {}
    try:
        with open("status_file.txt", "r", encoding="utf-8") as file:
            for line in file:
                splitted = line.split(maxsplit=-2)
                path = " ".join(splitted[:-2])
                status = splitted[-1]
                file_status[path] = "Disponível" if status == "complete" else "Em transferência" 

        return file_status
    except FileNotFoundError:
        return {}
