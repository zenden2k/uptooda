function PreUpload(taskUnion, reserved) {
    if (taskUnion.type() == "TypeFile") {
        local task = taskUnion.getFileTask();
        local ext = GetFileExtension(task.getFileName());
        if (ext.tolower() == "png") {
            local displayName = ExtractFileNameNoExt(task.getDisplayName());
            local tempName = GetTempDirectory() + displayName + "_" + GetCurrentThreadId() + "_" + random() % 50000 + ".png";
            if (CopyFile(task.getFileName(), tempName, true)) {
                task.setStatusText("Optimizing png...");
                task.addTempFile(tempName);
                local process = Process("optipng.exe", true);
                process.setArguments([tempName]);
                process.setHidden(true);
                process.start();

                local exitCode = process.waitForExit();
                if (exitCode == 0) {
                    // Success
                    task.setFileName(tempName);
                    task.setDisplayName(displayName + ".png")
                } else {
                    WriteLog("error", "Optipng returned code " + exitCode);
                    return false;
                }
            } else {
                WriteLog("error", "Cannot copy file to " + tempName);
                return false;
            }
        }
    }

    return true;
}

function PostUpload(task, reserved) {
    return true;
}