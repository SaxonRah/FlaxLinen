using Flax.Build;

public class LinenFlaxEditorTarget : GameProjectEditorTarget
{
    /// <inheritdoc />
    public override void Init()
    {
        base.Init();

        // Reference the modules for editor
        Modules.Add("LinenFlax");
        Modules.Add("LinenFlaxEditor");
    }
}
