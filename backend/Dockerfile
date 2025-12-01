FROM gradle:8.7-jdk17 AS builder

WORKDIR /workspace

COPY . .

RUN gradle --no-daemon bootJar \
    && JAR_FILE=$(ls build/libs | grep -v plain | head -n 1) \
    && mv build/libs/$JAR_FILE app.jar

FROM eclipse-temurin:17-jre

WORKDIR /app

COPY --from=builder /workspace/app.jar app.jar

EXPOSE 8080

ENTRYPOINT ["java", "-jar", "app.jar"]
