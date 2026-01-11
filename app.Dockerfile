FROM scratch
COPY . /
USER ion
WORKDIR /app
EXPOSE 8443
ENTRYPOINT ["/app/ion-server"]
