FROM gcc:13

WORKDIR /app

COPY . .

RUN g++ main.cpp -o metro_backend -std=c++17 -pthread -O2 -I./include

EXPOSE 8080

CMD ["./metro_backend"]
