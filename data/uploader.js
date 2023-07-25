var Upload = function(file, url) {
    this.file = file;
    this.url = url;
}

Upload.prototype.getType = function() {
    return this.file.type;
}

Upload.prototype.getSize = function() {
    return this.file.size;
}

Upload.prototype.getName = function() {
    return this.file.name;
}

Upload.prototype.doUpload = function() {
    var that = this;
    var formData = new FormData();

    formData.append("file", this.file, this.getName());
    formData.append("upload_file", true);

    $.ajax({
        type: "POST",
        url : this.url,
        xhr: function() {
            var myXhr = $.ajaxSettings.xhr();
            if(myXhr.upload) {
                myXhr.upload.addEventListner('progress', that.progressHandling, false);
            }
            return myXhr;
        },
        success: function(msg) {
            toastr.success("Der Upload der Konfiguration war erfolgreich", "Übertragung erfolgreich",   {timeOut: 5000});
        },
        error: function(msg) {

        },
        async: true,
        data: formData,
        cache: false,
        contentType: false,
        processData: false,
        timeout: 60000
    });
}
Upload.prototype.progressHandling = function(event) {
    var percent = 0;
    var position = event.loaded || event.position;
    var total = event.total;
    var progress_bar_id = "#progress-wrp";
    if(event.lengthComputable) {
        percent = Math.ceil(position / total * 100);
    }
    $(progress_bar_id + " .progress-bar").css("width", +percent + "%");
    $(progress_bar_id + " .status").text(+percent + "%");
};
